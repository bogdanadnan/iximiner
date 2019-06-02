//
// Created by Haifa Bogdan Adnan on 03/08/2018.
//

#include "../common/common.h"
#include "../app/arguments.h"
#include "client.h"

#include "simplejson/json.h"

pool_client::pool_client(arguments &args, get_status_ptr get_status) : __pool_settings_provider(args) {
    __worker_id = args.uid();
    __worker_name = args.name();
    __timestamp = microseconds();
    __show_pool_requests = args.show_pool_requests();
    __is_devfee_time = false;
    __get_status = get_status;
    __miner_version = arguments::get_app_version();
}

pool_update_result pool_client::update(int hash_rate) {
    pool_update_result result;
    result.success = false;

    pool_settings &settings = __get_pool_settings();

    if(settings.is_devfee) {
        hash_rate = hash_rate / 100;
    }

    string url = settings.pool_address + "/getminingblock?algo=0&id=" + __worker_id + "&worker=" +
            __worker_name + "&wallet=" + settings.wallet + "&hr=" + to_string(hash_rate) + "&miner=" +
            __miner_version;

    string response;
    if(settings.pool_extensions.find("Details") != string::npos) {
        string payload = "";

        if(__get_status != NULL)
            payload = __get_status();

        if(!payload.empty()) {
            if(__show_pool_requests)
                LOG("--> Pool request: " + url + "/" + payload);

            response = _http_post(url, payload, "application/json");
        }
        else {
            if(__show_pool_requests)
                LOG("--> Pool request: " + url);

            response = _http_get(url);
        }
    }
    else {
        if(__show_pool_requests)
            LOG("--> Pool request: " + url);

        response = _http_get(url);
    }

    if(__show_pool_requests)
        LOG("--> Pool response: " + response);

    if(!__validate_response(response)) {
        LOG("Error connecting to " + settings.pool_address + ".");
        return result;
    }

    json::JSON info = json::JSON::Load(response);

    result.success = !info.IsNull() && info["error"].IsNull();

    if(info.hasKey("version")) {
        string version = info["version"].ToString();
        if(version != settings.pool_version) {
            LOG("Connected to pool: " + version);
        }
        result.pool_version = settings.pool_version = version;
    }
    if(info.hasKey("extensions")) {
        result.extensions = settings.pool_extensions = info["extensions"].ToString();
    }

    if (result.success) {
        json::JSON data = info["result"];
        result.height = (uint32_t)data["num"].ToInt();
        result.version = (uint32_t)data["ver"].ToInt();
        if(data["dif"].JSONType() == json::JSON::Class::String) { // difficulty is sent as string instead of number
            string diff = data["dif"].ToString();
            result.difficulty = strtoull(diff.c_str(), NULL, 10);
        }
        else {
            result.difficulty = data["dif"].ToInt();
        }
        result.block_checksum = data["chk"].ToString();
        result.solver_address = data["adr"].ToString();
        result.recommendation = "mine"; //data["recommendation"].ToString();

//DEBUG
//        result.block_checksum = "RanOEZTpbvK4lqiVLHMvWcC2XARZLaWRBPBlSRU4Wq+m9p9dljB+npJfM6o=";
//        result.solver_address = "AUWgIEMwWJxIepw3U8BwnTWAkKtNWKkKlbPwWaGabwoVmrvKjosYfMKu8Y0uM0tU";
//        result.difficulty = 1844046597414253;
    }

    return result;
}

pool_submit_result pool_client::submit(const string &nonce, uint64_t height) {
    pool_submit_result result;
    result.success = false;

    pool_settings &settings = __get_pool_settings();

    string url = settings.pool_address + "/submitminingsolution?id=" + __worker_id + "&worker=" +
        __worker_name + "&wallet=" + settings.wallet + "&nonce=" + nonce + "&blocknum=" + to_string(height);

    if(__show_pool_requests)
        LOG("--> Pool request: " + url);

    string response = _http_get(url);

    if(!response.empty()) {
        if (__show_pool_requests)
            LOG("--> Pool response: " + response);

        if (!__validate_response(response)) {
            LOG("Error connecting to " + settings.pool_address + ".");
            return result;
        }

        json::JSON info = json::JSON::Load(response);
        result.success = !info.IsNull() && info["error"].IsNull();
    }
    else {
        result.success = true;
    }

    return result;
}

bool pool_client::__validate_response(const string &response) {
    return !response.empty() && response.find("result") != string::npos;
}

pool_settings &pool_client::__get_pool_settings() {
    pool_settings &user_settings = __pool_settings_provider.get_user_settings();

    if(user_settings.pool_extensions.find("Proxy") != string::npos) { // disable dev fee when connected to proxy
        return user_settings;
    }

    uint64_t minutes = (microseconds() - __timestamp) / 60000000;

    if(minutes != 0 && (minutes % 100 == 0)) {
        if(!__is_devfee_time) {
            LOG("--> Switching to dev wallet for 1 minute.");
            __is_devfee_time = true;
        }
    }
    else {
        if(__is_devfee_time) {
            LOG("--> Switching back to client wallet.");
            __is_devfee_time = false;
        }
    }

    if(!__is_devfee_time)
        return __pool_settings_provider.get_user_settings();
    else
        return __pool_settings_provider.get_dev_settings();
}

void pool_client::disconnect() {
    pool_settings &settings = __pool_settings_provider.get_user_settings();
    if(settings.pool_extensions.find("Disconnect") != string::npos) { // only send disconnect if pool supports it
        string url = settings.pool_address + "/disconnect&id=" + __worker_id + "&worker=" + __worker_name;
        _http_get(url);
    }
}
