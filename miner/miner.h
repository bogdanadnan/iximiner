//
// Created by Haifa Bogdan Adnan on 03/08/2018.
//

#ifndef IXIMINER_H
#define IXIMINER_H

#include "../http/client.h"
#include "../app/runner.h"

class miner : public runner {
public:
    miner(arguments &args);
    ~miner();

    virtual void run();
    virtual void stop();

    string get_status();

	static string calc_hash_ceil(uint64_t difficulty);
	static bool check_hash(const string &hash, const string &hash_ceil);

private:
    bool __update_pool_data();
    bool __display_report();
    void __disconnect_from_pool();

    string __recommendation;
    uint64_t __height;
    int __version;
    uint64_t __difficulty;
    string __block_checksum;
    string __solver_address;
    string __hash_ceil;
    uint32_t __found;
	uint32_t __confirmed;
	uint32_t __rejected;
    int __hs_threshold_hit;
    int __blocks_count;
	uint64_t __display_hits;

    time_t __begin_time;

    bool __running;

    arguments &__args;
    pool_client __client;
};
#endif //IXIMINER_H
