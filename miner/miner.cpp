//
// Created by Haifa Bogdan Adnan on 03/08/2018.
//

#include "../common/common.h"
#include "../app/arguments.h"
#include "../hash/hasher.h"

#include "../crypt/sha512.h"
#include "../crypt/base64.h"
#include "../crypt/hex.h"
#include "mini-gmp/mini-gmp.h"

#include "miner.h"
#include "miner_api.h"

miner::miner(arguments &args) : __args(args), __client(args, [&]() { return this->get_status(); }) {
    __height = 0;
    __version = 0;
    __difficulty = 0;
    __block_checksum = "";
    __solver_address = "";
    __hash_ceil = "";
    __found = 0;
    __confirmed = 0;
    __rejected = 0;
    __begin_time = time(NULL);
    __running = false;
    __hs_threshold_hit = 0;
    __running = false;
    __display_hits = 0;

    LOG("Miner name: " + __args.name());
    LOG("Wallet: " + __args.wallet());
    LOG("Pool address: " + __args.pool());

    vector<hasher*> hashers = hasher::get_hashers();
	for (vector<hasher*>::iterator it = hashers.begin(); it != hashers.end(); ++it) {
		if ((*it)->get_type() == "CPU") {
			if ((*it)->initialize()) {
				(*it)->configure(__args);
			}
			LOG("Compute unit: " + (*it)->get_type());
			LOG((*it)->get_info());
		}
	}

	vector<hasher *> selected_gpu_hashers;
	vector<string> requested_hashers = args.gpu_optimization();
	for (vector<hasher*>::iterator it = hashers.begin(); it != hashers.end(); ++it) {
		if ((*it)->get_type() == "GPU") {
            if(requested_hashers.size() > 0) {
                if(find(requested_hashers.begin(), requested_hashers.end(), (*it)->get_subtype()) != requested_hashers.end()) {
                    selected_gpu_hashers.push_back(*it);
                }
            }
            else {
                if (selected_gpu_hashers.size() == 0 || selected_gpu_hashers[0]->get_priority() < (*it)->get_priority()) {
                    selected_gpu_hashers.clear();
                    selected_gpu_hashers.push_back(*it);
                }
            }
		}
	}

	if (selected_gpu_hashers.size() > 0) {
        for (vector<hasher*>::iterator it = selected_gpu_hashers.begin(); it != selected_gpu_hashers.end(); ++it) {
            if ((*it)->initialize()) {
                (*it)->configure(__args);
            }
            LOG("Compute unit: " + (*it)->get_type() + " - " + (*it)->get_subtype());
            LOG((*it)->get_info());
        }
	}

	LOG("\n");

    __update_pool_data();
    vector<hasher*> active_hashers = hasher::get_active_hashers();

    for (vector<hasher *>::iterator it = active_hashers.begin(); it != active_hashers.end(); ++it) {
        (*it)->set_input(__height, __block_checksum, __solver_address, __recommendation);
    }

    __blocks_count = 1;
}

miner::~miner() {

}

void miner::run() {
    uint64_t last_update, last_report;
    miner_api miner_api(__args, *this);
    last_update = last_report = 0;

    vector<hasher *> hashers = hasher::get_active_hashers();

    if(hashers.size() == 0) {
        LOG("No hashers available. Exiting.");
    }
    else {
        __running = true;
    }

    while (__running) {
        for (vector<hasher *>::iterator it = hashers.begin(); it != hashers.end(); ++it) {
            if(!(*it)->is_running()) {
                __running = false;
                break;
            }
            vector<hash_data> hashes = (*it)->get_hashes();

            for (vector<hash_data>::iterator hash = hashes.begin(); hash != hashes.end(); hash++) {
                if (hash->block_checksum != __block_checksum) //the block expired
                    continue;

                bool result = miner::check_hash(hash->hash, __hash_ceil);
                if (result) {
                    if (__args.is_verbose())
                        LOG("--> Submitting nonce: " + hash->nonce);
                    pool_submit_result reply = __client.submit(hash->nonce, __height);
                    if (reply.success) {
                        if (false) { // TODO check what is needed to see if this is block or share
                            if (__args.is_verbose()) LOG("--> Block found.");
                            __found++;
                        } else {
                            if (__args.is_verbose()) LOG("--> Nonce confirmed.");
                            __confirmed++;
                        }
                    } else {
                        if (__args.is_verbose()) {
                            LOG("--> The nonce did not confirm.");
                            LOG("--> Pool response: ");
                            LOG(reply.pool_response);
                        }
                        __rejected++;
                        if (hash->realloc_flag != NULL)
                            *(hash->realloc_flag) = true;
                    }
                }
            }
        }

        if (microseconds() - last_update > __args.update_interval()) {
            if (__update_pool_data() || __recommendation == "pause") {
                for (vector<hasher *>::iterator it = hashers.begin(); it != hashers.end(); ++it) {
                    (*it)->set_input(__height, __block_checksum, __solver_address, __recommendation);
                }

                if(__recommendation != "pause")
                    __blocks_count++;
            }
            last_update = microseconds();
        }

        if (microseconds() - last_report > __args.report_interval()) {
            if(!__display_report())
                __running = false;

            last_report = microseconds();
        }

        this_thread::sleep_for(chrono::milliseconds(100));
    }

    for (vector<hasher *>::iterator it = hashers.begin(); it != hashers.end(); ++it) {
        (*it)->cleanup();
    }

    __disconnect_from_pool();
}

string miner::calc_hash_ceil(uint64_t difficulty) {
    unsigned char hash_ceil[10];
    hash_ceil[0] = 0x00;
    hash_ceil[1] = 0x00;
    for (int i = 0; i < 8; i++)
    {
        int shift = 8 * (7 - i);
        uint64_t mask = ((uint64_t)0xff) << shift;
        unsigned char cb = (unsigned char)((difficulty & mask) >> shift);
        hash_ceil[i + 2] = ~cb;
    }
    char output[30];
    hex::encode(hash_ceil, 10, output);
    return string(output);
}

bool miner::check_hash(const string &hash, const string &hash_ceil) {
    unsigned char byte_hash[32];
    unsigned char byte_hash_ceil[10];
    int hash_sz = hex::decode(hash.c_str(), byte_hash, 32);
    int hash_ceil_sz = hex::decode(hash_ceil.c_str(), byte_hash_ceil, 10);

    if (hash_sz < 32)
    {
        return false;
    }
    for (int i = 0; i < hash_sz; i++)
    {
        unsigned char cb = i < hash_ceil_sz ? byte_hash_ceil[i] : (unsigned char)0xff;
        if (cb > byte_hash[i]) return true;
        if (cb < byte_hash[i]) return false;
    }
    return true;
}

bool miner::__update_pool_data() {
    vector<hasher*> hashers = hasher::get_active_hashers();

    double hash_rate = 0;
    for(vector<hasher*>::iterator it = hashers.begin();it != hashers.end();++it) {
        hash_rate += (*it)->get_avg_hash_rate();
    }

    pool_update_result new_settings = __client.update(hash_rate);
    if(!new_settings.success) {
    	__recommendation = "pause";
    }
    if (new_settings.success &&
        (new_settings.height != __height ||
        new_settings.block_checksum != __block_checksum ||
        new_settings.solver_address != __solver_address ||
        new_settings.difficulty != __difficulty ||
        new_settings.version != __version ||
        new_settings.recommendation != __recommendation)) {
        __height = new_settings.height;
        __block_checksum = new_settings.block_checksum;
        __solver_address = new_settings.solver_address;
        __difficulty = new_settings.difficulty;
        __version = new_settings.version;
        __recommendation = new_settings.recommendation;
        __hash_ceil = miner::calc_hash_ceil(__difficulty);

        if(__args.is_verbose()) {
            stringstream ss;
            ss << "-----------------------------------------------------------------------------------------------------------------------------------------" << endl;
            ss << "--> Pool data updated   Block: " << __height << "  Checksum: " << __block_checksum << endl;
            ss << "--> Solver: " << __solver_address << "  Difficulty: " << __difficulty << "  Miner: " << __args.name() << endl;
            ss << "-----------------------------------------------------------------------------------------------------------------------------------------";

            LOG(ss.str());
            __display_hits = 0;
        }
        return true;
    }

    return false;
}

bool miner::__display_report() {
    vector<hasher*> hashers = hasher::get_active_hashers();
    stringstream ss;

    double hash_rate = 0;
    double avg_hash_rate = 0;
    uint32_t hash_count = 0;

    time_t total_time = time(NULL) - __begin_time;

    stringstream header;
    stringstream log;

    for (vector<hasher *>::iterator it = hashers.begin(); it != hashers.end(); ++it) {
        hash_rate += (*it)->get_current_hash_rate();
        avg_hash_rate += (*it)->get_avg_hash_rate();
        hash_count += (*it)->get_hash_count();
    }

    header << "|TotalHR";
    log << "|" << setw(7) << (int)hash_rate;
    for (vector<hasher *>::iterator it = hashers.begin(); it != hashers.end(); ++it) {
        map<int, device_info> devices = (*it)->get_device_infos();
        for(map<int, device_info>::iterator d = devices.begin(); d != devices.end(); ++d) {
            header << "|" << ((d->first < 10) ? " " : "") << (*it)->get_type() << d->first;
            log << "|" << setw(5) << (int)(d->second.hashrate);
        }
    }
    header << "|Avg   |     Time|Acc   |Rej   |Block|";
    log << "|" << setw(6) << (int)avg_hash_rate
            << "|" << setw(9) << format_seconds(total_time)
            << "|" << setw(6) << __confirmed
            << "|" << setw(6) << __rejected
            << "|" << setw(5) << __found << "|";

    if((__display_hits % 10) == 0) {
        string header_str = header.str();
        string separator(header_str.size(), '-');

        if(__display_hits > 0)
            LOG(separator);

        LOG(header_str);
        LOG(separator);
    }

    LOG(log.str());

    if(__recommendation != "pause") {
        if (hash_rate <= __args.hs_threshold()) {
            __hs_threshold_hit++;
        } else {
            __hs_threshold_hit = 0;
        }
    }

    if(__hs_threshold_hit >= 5 && __blocks_count > 1) {
        LOG("Hashrate is lower than requested threshold, exiting.");
        exit(0);
    }

    __display_hits++;

    return true;
}

void miner::stop() {
    cout << endl << "Received termination request, please wait for cleanup ... " << endl;
    __running = false;
}

string miner::get_status() {
    stringstream ss;
    ss << "[ { \"name\": \"" << __args.name() << "\", \"block_height\": " << __height << ", \"time_running\": " << (time(NULL) - __begin_time) <<
       ", \"total_blocks\": " << __blocks_count << ", \"shares\": " << __confirmed << ", \"rejects\": " << __rejected <<
       ", \"earned\": " << __found << ", \"hashers\": [ ";

    vector<hasher*> hashers = hasher::get_active_hashers();

    for(vector<hasher*>::iterator h = hashers.begin(); h != hashers.end();) {
        ss << "{ \"type\": \"" << (*h)->get_type() << "\", \"subtype\": \"" << (*h)->get_subtype() << "\", \"devices\": [ ";
        map<int, device_info> devices = (*h)->get_device_infos();
        for(map<int, device_info>::iterator d = devices.begin(); d != devices.end();) {
            ss << "{ \"id\": " << d->first << ", \"bus_id\": \"" << d->second.bus_id << "\", \"name\": \"" << d->second.name << "\", \"intensity\": " << d->second.intensity <<
                ", \"hashrate\": " << d->second.hashrate << " }";
            if((++d) != devices.end())
                ss << ", ";
        }
        ss << " ] }";

        if((++h) != hashers.end())
            ss << ", ";
    }

    ss << " ] } ]";

    return ss.str();
}

void miner::__disconnect_from_pool() {
    __client.disconnect();
}