//
// Created by Haifa Bogdan Adnan on 04/08/2018.
//

#include "../common/common.h"
#include "http_parser/http_parser.h"

#include <cpr/cpr.h>

#include "http.h"

#ifdef _WIN64
#define close closesocket
#endif

struct http_callback_data {
    string body;
    bool complete;
};

int http_callback (http_parser* parser, const char *at, size_t length) {
    http_callback_data *data = (http_callback_data *)parser->data;
    data->body += string(at, length);
    return 0;
}

int http_complete_callback (http_parser* parser) {
    http_callback_data *data = (http_callback_data *)parser->data;
    data->complete = true;
    return  0;
}

struct http_data {
public:
    http_data(const string &uri, const string &data) {
        host = uri;

        protocol = "http";

        if(host.find("http://") != string::npos) {
            host = host.erase(0, 7);
            protocol = "http";
        }

        if(host.find("https://") != string::npos) {
            host = host.erase(0, 8);
            protocol = "https";
        }

        if(host.find("/") != string::npos) {
            path = host.substr(host.find("/"));
            host = host.erase(host.find("/"));
        }
        else {
            path = "/";
        }

        if(path.find("?") != string::npos) {
            query = path.substr(path.find("?"));
            path = path.erase(path.find("?"));
            query.erase(0, 1);
        }

        string port_str = "";
        if(host.find(":") != string::npos) {
            port_str = host.substr(host.find(":"));
            host = host.erase(host.find(":"));
        }

        port = 80;
        if(port_str != "") {
            if(port_str.find(":") != string::npos) {
                port_str = port_str.erase(port_str.find(":"), 1);
                port = atoi(port_str.c_str());
            }
        }

        action = "GET";
        if(data != "") {
            payload = data;
            action = "POST";
        }
    }

    string protocol;
    string host;
    int port;
    string action;
    string path;
    string query;
    string payload;
};

int http::__socketlib_reference = 0;

http::http() {
#ifdef _WIN64
    if(__socketlib_reference == 0) {
        WSADATA wsaData;
        int iResult;

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            LOG("WSAStartup failed:"+ to_string(iResult));
            exit(1);
        }
	}
#endif
    __socketlib_reference++;
}

http::~http() {
    __socketlib_reference--;
#ifdef _WIN64
    if(__socketlib_reference == 0) {
    	WSACleanup();
	}
#endif
}

vector<string> http::_resolve_host(const string &hostname)
{
    string host = hostname;

    if(host.find(":") != string::npos) {
        host = host.erase(host.rfind(":"));
    }

    addrinfo hints, *servinfo, *p;
    sockaddr_in *h;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if(getaddrinfo( host.c_str() , "http" , &hints , &servinfo) != 0) {
        return vector<string>();
    }

    vector<string> addresses;
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        h = (sockaddr_in *) p->ai_addr;
        string ip = inet_ntoa(h->sin_addr);
        if(ip != "0.0.0.0")
            addresses.push_back(ip);
    }

    freeaddrinfo(servinfo);
    return addresses;
}

string http::_encode(const string &src) {
    string new_str = "";
    char c;
    int ic;
    const char* chars = src.c_str();
    char bufHex[10];
    int len = strlen(chars);

    for(int i=0;i<len;i++){
        c = chars[i];
        ic = c;
        if (c==' ') new_str += '+';
        else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') new_str += c;
        else {
            sprintf(bufHex,"%X",c);
            if(ic < 16)
                new_str += "%0";
            else
                new_str += "%";
            new_str += bufHex;
        }
    }
    return new_str;
}

string http_internal_impl::__get_response(const string &url, const string &post_data, const string &content_type) {
    http_callback_data reply;
    reply.complete = false;

    http_data query(url, post_data);
    if(query.protocol != "http")
        return "";

    vector<string> ips = _resolve_host(query.host);
    for(int i=0;i<ips.size();i++) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(query.port);
        inet_pton(AF_INET, ips[i].c_str(), &addr.sin_addr);

        if(connect(sockfd,(struct sockaddr *) &addr, sizeof (addr)) != 0) {
            close(sockfd);
            continue;
        }

#ifdef _WIN64
        u_long nonblock = 1;
        ioctlsocket(sockfd, FIONBIO, &nonblock);
#else
        int flags;
        flags = fcntl(sockfd,F_GETFL,0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#endif

        string request = query.action + " " + query.path + ((query.query == "") ? "" : ("?" + query.query)) + " HTTP/1.1\r\nHost: " + query.host + "\r\n";
        if(query.payload != "") {
            request += "Content-Type: application/" + content_type + "\r\nContent-Length: " + to_string(query.payload.length()) + "\r\n\r\n" + query.payload + "\r\n";
        }
        request += "\r\n";

        char *buff = (char *)request.c_str();
        int sz = request.size();
        int n = 0;

        while(sz > 0) {
            n = send(sockfd, buff, sz, 0);
            if(n < 0) break;
            buff+=n;
            sz-=n;
        }

        if(n < 0) {
            close(sockfd);
            continue;
        }

        http_parser_settings settings;
        memset(&settings, 0, sizeof(settings));
        settings.on_body = http_callback;
        settings.on_message_complete = http_complete_callback;

        http_parser parser;
        http_parser_init(&parser, HTTP_RESPONSE);
        parser.data = (void *)&reply;

        fd_set fds;
        timeval tv;

        time_t timestamp = time(NULL);
        while(time(NULL) - timestamp < 10) {
            FD_ZERO(&fds);
            FD_SET(sockfd, &fds);

            tv.tv_sec = 0;
            tv.tv_usec = 100000;

            n = select(sockfd + 1, &fds, NULL, NULL, &tv);
            if(n == 0)
                continue;
            else if(n < 0)
                break;
            else {
                char buffer[2048];
                n = recv(sockfd, buffer, 2048, 0);
                if (n > 0)
                    http_parser_execute(&parser, &settings, buffer, n);
                else if(n <= 0)
                    break;

                if (reply.complete)
                    break;
            }
        }

        close(sockfd);

        if(reply.body != "")
            break;
    }

    return reply.body;
};

string http_internal_impl::_http_get(const string &url) {
    return __get_response(url, "", "");
}

string http_internal_impl::_http_post(const string &url, const string &post_data, const string &content_type) {
    return __get_response(url, post_data, content_type);
}

string http_cpr_impl::_http_get(const string &url) {
    auto r = cpr::Get(cpr::Url{url});
    return r.text;
}

string http_cpr_impl::_http_post(const string &url, const string &post_data, const string &content_type) {
    auto r = cpr::Post(cpr::Url{url},
                       cpr::Body{post_data},
                       cpr::Header{{"Content-Type", content_type}});

    return r.text;
}

