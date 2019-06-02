//
// Created by Haifa Bogdan Adnan on 03/08/2018.
//

#ifndef IXIMINER_HASHER_H
#define IXIMINER_HASHER_H

#include "argon2/defs.h"

struct hash_data {
    hash_data() {
        realloc_flag = NULL;
    };
    string nonce;
    string block_checksum;
    string solver_address;
    string base;
    string hash;
    bool *realloc_flag;
};

struct hash_timing {
    uint64_t time_info;
    size_t hash_count;
};

struct device_info {
	device_info() {
		hashcount = 0;
		hashrate = 0;
		intensity = 0;
	}

	string name;
	string bus_id;
	double intensity;
	double hashrate;
	size_t hashcount;
};

#define REGISTER_HASHER(x)        extern "C"  { DLLEXPORT void hasher_loader() { x *instance = new x(); } }

class DLLEXPORT hasher {
public:
    hasher();
    virtual ~hasher();

    virtual bool initialize() = 0;
    virtual bool configure(arguments &args) = 0;
    virtual void cleanup() = 0;

    string get_type();
	string get_subtype(bool short_name = false);
	int get_priority();
    string get_info();
    void set_input(uint64_t height, const string &block_checksum, const string &solver_address, const string &recommendation);

    double get_current_hash_rate();
    double get_avg_hash_rate();

    uint32_t get_hash_count();

    vector<hash_data> get_hashes();
    map<int, device_info> &get_device_infos();
    bool is_running();

    static vector<hasher*> get_hashers_of_type(const string &type);
    static vector<hasher*> get_hashers();
    static vector<hasher*> get_active_hashers();
    static void load_hashers();

protected:
    double _intensity;
    string _type;
	string _subtype;
	string _short_subtype; //max 3 characters
	int _priority;
    string _description;

	void _store_hash(const hash_data &hash, int device_id);
	void _store_hash(const vector<hash_data> &hashes, int device_id);

	void _store_device_info(int device_id, device_info device);

    hash_data _get_input();
    bool _should_pause();
    void _update_running_status(bool running);
	vector<string> _get_gpu_filters(arguments &args);

private:
    string __make_base(const string &block_checksum, const string &solver_address);
	void __update_hashrate();

    static vector<hasher*> *__registered_hashers;

    mutex __input_mutex;
    uint64_t __height;
    string __block_checksum;
    string __solver_address;
    bool __pause;
    bool __is_running;

    mutex __hashes_mutex;
    vector<hash_data> __hashes;

    uint64_t __hashrate_time;
    map<int, device_info> __device_infos;
    double __hashrate;

    size_t __total_hash_count;

    size_t __hash_count;
    uint64_t __begin_round_time;
    list<hash_timing> __hash_timings;
};

#endif //IXIMINER_HASHER_H
