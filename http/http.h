//
// Created by Haifa Bogdan Adnan on 04/08/2018.
//

#ifndef IXIMINER_HTTP_H
#define IXIMINER_HTTP_H


class http {
public:
    http();
    virtual ~http();

protected:
    virtual string _http_get(const string &url) { return ""; };
    virtual string _http_post(const string &url, const string &post_data, const string &content_type) { return ""; };
    string _encode(const string &src);
    vector<string> _resolve_host(const string &hostname);

private:
    static int __socketlib_reference;
};

class http_internal_impl : public http {
protected:
    virtual string _http_get(const string &url);
    virtual string _http_post(const string &url, const string &post_data, const string &content_type);

private:
    string __get_response(const string &url, const string &post_data, const string &content_type);
};

class http_cpr_impl : public http {
protected:
    virtual string _http_get(const string &url);
    virtual string _http_post(const string &url, const string &post_data, const string &content_type);

private:
    string __get_response(const string &url, const string &post_data, const string &content_type);
};

#endif //IXIMINER_HTTP_H
