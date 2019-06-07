//
// Created by Haifa Bogdan Adnan on 03/08/2018.
//

#ifndef IXIMINER_CLIENT_H
#define IXIMINER_CLIENT_H

#include "http.h"
#include "pool_settings_provider.h"

struct pool_result {
    bool success;
};

struct pool_update_result : public pool_result {
    int version;
    uint64_t height;
    string block_checksum;
    string solver_address;
    uint64_t difficulty;

    string recommendation;
    string extensions;
    string pool_version;

    bool update(pool_update_result &src) {
        if(version != src.version ||
                height != src.height ||
                difficulty != src.difficulty ||
                block_checksum != src.block_checksum ||
                solver_address != src.solver_address ||
                recommendation != src.recommendation ||
                extensions != src.extensions) {
            version = src.version;
            height = src.height;
            difficulty = src.difficulty;
            block_checksum = src.block_checksum;
            solver_address = src.solver_address;
            recommendation = src.recommendation;
            extensions = src.extensions;

            return true;
        }

        return false;
    }

    string response() {
        stringstream ss;

        ss << "{ \"result\": { \"num\": " << height << ", \"ver\": " << version
           << ", \"dif\": \"" << difficulty << "\", \"chk\": \"" << block_checksum << ", \"adr\": \"" << solver_address
           << "\", \"PoW field\": \"\" } , \"error\": null, \"id\": null, \"extensions\": \"" << extensions << "\", \"version\": \""
           << pool_version << "\" }";

        return ss.str();
    }
};

struct pool_submit_result : public pool_result {
    string pool_response;
};

typedef function<string ()> get_status_ptr;

class pool_client : public http_cpr_impl {
public:
    pool_client(arguments &args, get_status_ptr get_status);

    pool_update_result update(int hash_rate);
    pool_submit_result submit(const string &nonce, uint64_t height);
    void disconnect();

private:
    bool __validate_response(const string &response);
    pool_settings &__get_pool_settings();

    pool_settings_provider __pool_settings_provider;
    bool __is_devfee_time;
    string __miner_version;
    string __worker_id;
    string __worker_name;

    bool __show_pool_requests;

    uint64_t __timestamp;
    get_status_ptr __get_status;
};

#endif //IXIMINER_CLIENT_H
