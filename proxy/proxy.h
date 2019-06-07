//
// Created by Haifa Bogdan Adnan on 03/08/2018.
//

#ifndef PROJECT_PROXY_H
#define PROJECT_PROXY_H

#include "../app/runner.h"
#include "../app/arguments.h"
#include "../http/client.h"
#include "../http/node_api.h"

struct miner_hashrate {
    double hashrate;
    time_t timestamp;
};

struct miner_client {
    miner_client() {
        hashrate = 0;
        timestamp = 0;
        created = time(NULL);
    }
    string worker_name;
    double hashrate;
    time_t timestamp;
    time_t created;
    string details;

    list<miner_hashrate> hashrate_history;
};

struct global_status {
    global_status() {
        hashrate = 0;
        uptime = 0;
        shares = 0;
        rejects = 0;
        workers_count = 0;
        current_block = 0;
        blocks = 0;
    }

    double hashrate;
    time_t uptime;
    int shares;
    int rejects;
    int workers_count;
    int current_block;
    int blocks;
};

struct miner_list_item {
    miner_list_item() {};
    miner_list_item(miner_client &mc, time_t timestamp) {
        worker_name = mc.worker_name;
        hashrate = mc.hashrate;
        uptime = timestamp - mc.created;
    };

    string worker_name;
    double hashrate;
    time_t uptime;
};

struct miner_status {
    miner_status() {
        uptime = 0;
        hashrate = 0;
        shares = 0;
        rejects = 0;
        devices_count = 0;
        blocks = 0;
    };

    time_t uptime;
    double hashrate;
    int shares;
    int rejects;
    int devices_count;
    int blocks;
};

struct device_details {
    device_details() {
        hashrate = 0;
    }

    string hasher_name;
    string device_name;
    double hashrate;
};

class proxy : public runner {
public:
    proxy(arguments &args);
    ~proxy();

    virtual void run();
    virtual void stop();

    string process_info_request(const string &ip, const string &miner_id, const string &miner_name, double hashrate, const string &details);
    string process_submit_request(const string &ip, const string &miner_id, const string &miner_name, uint64_t height, const string &nonce);
    string process_disconnect_request(const string &ip, const string &miner_id, const string &miner_name);

    map<string, string> get_workers();

    string get_status();

    global_status get_global_status();
    account_balance get_account_balance();
    void get_global_hashrate_history(list<miner_hashrate> &history);
    void get_workers_list(vector<miner_list_item> &workers);

    miner_status get_worker_status(const string &worker_id);
    void get_worker_devices(const string &worker_id, vector<device_details> &devices);
    void get_worker_hashrate_history(const string &worker_id, list<miner_hashrate> &history);

private:
    bool __update_pool_data();
    void __update_global_history();

    mutex __pool_block_settings_lock;
    pool_update_result __pool_block_settings;

    mutex __miner_clients_lock;
    map<string, miner_client> __miner_clients;

    arguments &__args;
    bool __running;
    time_t __start;

    uint32_t __found;
    uint32_t __confirmed;
    uint32_t __rejected;

    mutex __global_hashrate_history_lock;
    list<miner_hashrate> __global_hashrate_history;

    pool_client __client;
};


#endif //PROJECT_PROXY_H
