//
// Created by Haifa Bogdan Adnan on 05/08/2018.
//

#ifndef ARIOMINER_ARGON2_H
#define ARIOMINER_ARGON2_H

#include "defs.h"
#include "../hasher.h"

typedef bool (*argon2_blocks_prehash)(void *, int, argon2profile *, void *);
typedef void *(*argon2_blocks_filler_ptr)(void *, int, argon2profile *, void *);
typedef bool (*argon2_blocks_posthash)(void *, int, argon2profile *, void *);

class DLLEXPORT argon2 {
public:
    argon2(argon2_blocks_prehash prehash, argon2_blocks_filler_ptr filler, argon2_blocks_posthash posthash, void *seed_memory, void *user_data);

    bool initialize_seeds(const argon2profile &profile);
    bool fill_blocks(const argon2profile &profile);
    bool encode_hashes(const argon2profile &profile);
    
    vector<hash_data> generate_hashes(const argon2profile &profile, hash_data &input);

    void set_seed_memory(uint8_t *memory);
    uint8_t *get_output_memory();
    void set_seed_memory_offset(size_t offset);
    void set_lane_length(int length); // in blocks
    void set_threads(int threads);
private:
	string __make_nonce();
    void __initial_hash(const argon2profile &profile, uint8_t *blockhash, const char *base, size_t base_sz, const char *salt, size_t salt_sz);
    void __fill_first_blocks(const argon2profile &profile, uint8_t *blockhash, int thread);

	argon2_blocks_prehash __prehash;
	argon2_blocks_filler_ptr __filler;
	argon2_blocks_posthash __posthash;

    int __threads;

    uint8_t *__seed_memory;
	uint8_t *__output_memory;

    size_t __seed_memory_offset;
    int __lane_length;
    void *__user_data;

    vector<hash_data> __inputs;
};


#endif //ARIOMINER_ARGON2_H
