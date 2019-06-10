//
// Created by Haifa Bogdan Adnan on 03/08/2018.
//

#include "../common/common.h"
#include "../crypt/base64.h"
#include "../crypt/hex.h"
#include "../crypt/random_generator.h"
#include "../app/arguments.h"

#include "../common/dllexport.h"
#include "argon2/argon2.h"
#include "hasher.h"

hasher::hasher() {
    _intensity = 0;
    _type = "";
	_subtype = "";
    _description = "";
	_priority = 0;

	__height = 0;
    __block_checksum = "";
    __solver_address = "";
    __base = "";
    __hash_ceil = "";

    __pause = false;
    __is_running = false;

    __begin_round_time = __hashrate_time = microseconds();
    __hashrate = 0;

    __total_hash_count = 0;

    __hash_count = 0;

    if(__registered_hashers == NULL) {
        __registered_hashers = new vector<hasher*>();
    }
    __registered_hashers->push_back(this);
}

hasher::~hasher() {

};

string hasher::get_type() {
	return _type;
}

string hasher::get_subtype(bool short_subtype) {
    if(short_subtype && !(_short_subtype.empty())) {
        string short_version = _short_subtype;
        short_version.erase(3);
        return short_version;
    }
    else
    	return _subtype;
}

int hasher::get_priority() {
	return _priority;
}

string hasher::get_info() {
    return _description;
}

void hasher::set_input(uint64_t height, const string &block_checksum, const string &solver_address, const string &recommendation, const string &hash_ceil) {
    bool new_block = false;
    __input_mutex.lock();
    new_block = height != __height;
    __height = height;
    __block_checksum = block_checksum;
    __solver_address = solver_address;
    __base = __make_base(block_checksum, solver_address);
    __hash_ceil = hash_ceil;
    __pause = (recommendation == "pause");
    __input_mutex.unlock();

    if(new_block) {
        uint64_t timestamp = microseconds();
        __hashes_mutex.lock();
        __hash_timings.push_back(hash_timing{timestamp - __begin_round_time, __hash_count});
        __hash_count = 0;
        __hashes_mutex.unlock();

        if (__hash_timings.size() > 20) //we average over 20 blocks
            __hash_timings.pop_front();
        __begin_round_time = timestamp;
    }
}

hash_data hasher::_get_input() {
    string tmp_block_checksum = "";
    string tmp_hash_ceil = "";
    string tmp_base = "";
    __input_mutex.lock();
    tmp_block_checksum = __block_checksum;
    tmp_hash_ceil = __hash_ceil;
    tmp_base = __base;
    __input_mutex.unlock();

    hash_data new_hash;
    new_hash.nonce = "";
    new_hash.block_checksum = tmp_block_checksum;
    new_hash.hash_ceil = tmp_hash_ceil;
    new_hash.base = tmp_base;

    return new_hash;
}

double hasher::get_current_hash_rate() {
    double hashrate = 0;
    __hashes_mutex.lock();
    __update_hashrate();
    hashrate = __hashrate;
    __hashes_mutex.unlock();
    return hashrate;
}

double hasher::get_avg_hash_rate() {
    size_t total_hashes = 0;
    uint64_t total_time = 0;
    for(list<hash_timing>::iterator it = __hash_timings.begin(); it != __hash_timings.end();it++) {
        total_time += it->time_info;
        total_hashes += it->hash_count;
    }
    total_time += (microseconds() - __begin_round_time);
    __hashes_mutex.lock();
    total_hashes += __hash_count;
    __hashes_mutex.unlock();

    if(total_time == 0)
        return 0;
    else
        return total_hashes / (total_time / 1000000.0);
}

uint32_t hasher::get_hash_count() {
    return __total_hash_count;
}

vector<hash_data> hasher::get_hashes() {
    vector<hash_data> tmp;
    __hashes_mutex.lock();
    tmp.insert(tmp.end(), __hashes.begin(), __hashes.end());
    __hashes.clear();
    __hashes_mutex.unlock();
    return tmp;
}

void hasher::_store_hash(int hash_count, const vector<hash_data> &succeded, int device_id) {
	if (hash_count == 0) return;

	__hashes_mutex.lock();
	__hashes.insert(__hashes.end(), succeded.begin(), succeded.end());

	__hash_count += hash_count;
	__device_infos[device_id].hashcount += hash_count;

    __total_hash_count += hash_count;

	__update_hashrate();

//	for(int i=0;i<hashes.size();i++)
//	    LOG(hashes[i].hash);

	__hashes_mutex.unlock();
}

void hasher::__update_hashrate() {
    uint64_t timestamp = microseconds();

    if (timestamp - __hashrate_time > 10000000) { //we calculate hashrate every 10 seconds
        size_t hashcount = 0;
        for(map<int, device_info>::iterator iter = __device_infos.begin(); iter != __device_infos.end(); ++iter) {
            hashcount += iter->second.hashcount;
            iter->second.hashrate = iter->second.hashcount / ((timestamp - __hashrate_time) / 1000000.0);
            iter->second.hashcount = 0;
        }
        __hashrate = hashcount / ((timestamp - __hashrate_time) / 1000000.0);
        __hashrate_time = timestamp;
    }
}

vector<hasher *> hasher::get_hashers() {
    return *__registered_hashers;
}

vector<hasher *> hasher::get_active_hashers() {
    vector<hasher *> filtered;
    for(vector<hasher*>::iterator it = __registered_hashers->begin();it != __registered_hashers->end();++it) {
        if((*it)->_intensity != 0)
            filtered.push_back(*it);
    }
    return filtered;
}

bool hasher::_should_pause() {
    bool pause = false;
    __input_mutex.lock();
    pause = __pause;
    __input_mutex.unlock();

    return pause;
}

string hasher::__make_base(const string &block_checksum, const string &solver_address) {
    unsigned char blk[256];
    unsigned char slv[256];
    char output[512];
    int blk_sz = base64::decode(block_checksum.c_str(), (char *)blk, 256);
    int slv_sz = base64::decode(solver_address.c_str(), (char *)slv, 256);

    unsigned char input[512];
    memcpy(input, blk, blk_sz);
    memcpy(input + blk_sz, slv, slv_sz);

    hex::encode(input, blk_sz + slv_sz, output);

    return string(output);
}

vector<hasher*> *hasher::__registered_hashers = NULL;

typedef void *(*hasher_loader)();

void hasher::load_hashers() {
	string module_path = arguments::get_app_folder() + "/modules/";
	vector<string> files = get_files(module_path);
	for(vector<string>::iterator iter = files.begin();iter != files.end();iter++) {
		if(iter->find(".hsh") != string::npos) {
			void *__dll_handle = dlopen((module_path + *iter).c_str(), RTLD_LAZY);
			if(__dll_handle != NULL) {
				hasher_loader hasher_loader_ptr = (hasher_loader) dlsym(__dll_handle, "hasher_loader");
				(*hasher_loader_ptr)();
			}
		}
	}
}

bool hasher::is_running() {
    return __is_running;
}

void hasher::_update_running_status(bool running) {
    __is_running = running;
}

vector<string> hasher::_get_gpu_filters(arguments &args) {
    vector<string> local_filters = args.gpu_filter();
    vector<hasher*> gpu_hashers = get_hashers_of_type("GPU");
    for(vector<string>::iterator it = local_filters.end(); it-- != local_filters.begin();) {
        string filter = *it;
        string filter_type = "";
        for(vector<hasher*>::iterator hit = gpu_hashers.begin(); hit != gpu_hashers.end(); hit++) {
            if(filter.find((*hit)->_subtype + ":") == 0) {
                filter_type = (*hit)->_subtype;
                break;
            }
        }
        if(filter_type != "" && filter_type != this->_subtype) {
            local_filters.erase(it);
        }
        else if(filter_type != "") { //cleanup subtype prefix
            it->erase(0, this->_subtype.size() + 1);
        }
    }
    return local_filters;
}

vector<hasher *> hasher::get_hashers_of_type(const string &type) {
    vector<hasher *> filtered;
    for(vector<hasher*>::iterator it = __registered_hashers->begin();it != __registered_hashers->end();++it) {
        if((*it)->_type == type)
            filtered.push_back(*it);
    }
    return filtered;
}

map<int, device_info> &hasher::get_device_infos() {
//    map<int, device_info> device_infos_copy;
//    __hashes_mutex.lock();
//    device_infos_copy.insert(__device_infos.begin(), __device_infos.end());
//    __hashes_mutex.unlock();
    return __device_infos;
}

void hasher::_store_device_info(int device_id, device_info device) {
    __hashes_mutex.lock();
    __device_infos[device_id] = device;
    __hashes_mutex.unlock();
}
