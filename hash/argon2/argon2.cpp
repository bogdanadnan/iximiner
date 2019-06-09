//
// Created by Haifa Bogdan Adnan on 05/08/2018.
//

#include "../../common/common.h"
#include "../../crypt/base64.h"
#include "../../crypt/hex.h"
#include "../../crypt/random_generator.h"

#include "../../app/arguments.h"

#include "blake2/blake2.h"
#include "../../common/dllexport.h"
#include "argon2.h"
#include "defs.h"

argon2::argon2(argon2_blocks_prehash prehash, argon2_blocks_filler_ptr filler, argon2_blocks_posthash posthash, void *seed_memory, void *user_data) {
    __prehash = prehash;
    __filler = filler;
    __posthash = posthash;
    __output_memory = __seed_memory = (uint8_t*)seed_memory;
    __seed_memory_offset = argon2profile_default->memsize;
    __lane_length = -1;
    __user_data = user_data;
    __inputs = NULL;
    __threads = 0;
    set_threads(1);
}

void argon2::set_threads(int threads) {
    if(threads == __threads) return;
    __threads = threads;
    if(__inputs != NULL)
        free(__inputs);
    __inputs = (uint8_t*)malloc(threads * IXIAN_NONCE_SIZE);
}

int argon2::generate_hashes(const argon2profile &profile, hash_data &input, vector<hash_data> &results) {
    random_generator::instance().get_random_data(__inputs, __threads * IXIAN_NONCE_SIZE);

    if(initialize_seeds(profile, input)) {
        if(fill_blocks(profile)) {
            return encode_hashes(profile, input, results);
        }
    }

    return 0;
}

bool argon2::initialize_seeds(const argon2profile &profile, hash_data &input) {
    unsigned char base[256];
    size_t base_sz = hex::decode(input.base.c_str(), base, 256);

    if(__prehash != NULL) {
        for (int i = 0; i < __threads; i++) {
            memcpy(__seed_memory + i * IXIAN_SEED_SIZE, base, base_sz);
            memcpy(__seed_memory + i * IXIAN_SEED_SIZE + base_sz, __inputs + i * IXIAN_NONCE_SIZE, IXIAN_NONCE_SIZE);
        }

        return (*__prehash)(__seed_memory, __threads, (argon2profile*)&profile, __user_data);
    }
    else {
        uint8_t blockhash[ARGON2_PREHASH_SEED_LENGTH];

        for (int i = 0; i < __threads; i++) {
            __initial_hash(profile, blockhash, (char *) base, base_sz, (char *) __inputs + i * IXIAN_NONCE_SIZE, IXIAN_NONCE_SIZE);

            memset(blockhash + ARGON2_PREHASH_DIGEST_LENGTH, 0,
                   ARGON2_PREHASH_SEED_LENGTH -
                   ARGON2_PREHASH_DIGEST_LENGTH);

            __fill_first_blocks(profile, blockhash, i);
        }

        return true;
    }
}

bool argon2::fill_blocks(const argon2profile &profile) {
    __output_memory = (uint8_t *)(*__filler) (__seed_memory, __threads, (argon2profile*)&profile, __user_data);
    return __output_memory != NULL;
}

int argon2::encode_hashes(const argon2profile &profile, hash_data &input, vector<hash_data> &results) {
    char encoded_hash[ARGON2_RAW_LENGTH * 2 + 1];
    char encoded_nonce[IXIAN_NONCE_SIZE * 2 + 1];
    uint8_t byte_hash_ceil[10];
    int hash_ceil_sz = hex::decode(input.hash_ceil.c_str(), byte_hash_ceil, 10);

    if(__posthash != NULL) {
        if((*__posthash)(__seed_memory, __threads, (argon2profile*)&profile, __user_data)) {

            uint8_t *hash = NULL;
            for (int i = 0; i < __threads; i++) {
                hash = __seed_memory + i * ARGON2_RAW_LENGTH;
                if(__check_hash(hash, ARGON2_RAW_LENGTH, byte_hash_ceil, hash_ceil_sz)) {
                    hex::encode(hash, ARGON2_RAW_LENGTH, encoded_hash);
                    hex::encode(__inputs + i * IXIAN_NONCE_SIZE, IXIAN_NONCE_SIZE, encoded_nonce);
                    input.hash = encoded_hash;
                    input.nonce = encoded_nonce;
                    results.push_back(input);
                }
            }

            return __threads;
        }
        return 0;
    }
    else {
        unsigned char raw_hash[ARGON2_RAW_LENGTH];

        if (__output_memory != NULL) {
            for (int i = 0; i < __threads; i++) {
                blake2b_long((void *) raw_hash, ARGON2_RAW_LENGTH,
                             (void *) (__output_memory + i * __seed_memory_offset), ARGON2_BLOCK_SIZE);


                if(__check_hash(raw_hash, ARGON2_RAW_LENGTH, byte_hash_ceil, hash_ceil_sz)) {
                    hex::encode(raw_hash, ARGON2_RAW_LENGTH, encoded_hash);
                    hex::encode(__inputs + i * IXIAN_NONCE_SIZE, IXIAN_NONCE_SIZE, encoded_nonce);
                    input.hash = encoded_hash;
                    input.nonce = encoded_nonce;
                    results.push_back(input);
                }
            }
            return __threads;
        }
        else
            return 0;
    }
}

void argon2::__initial_hash(const argon2profile &profile, uint8_t *blockhash, const char *base, size_t base_sz, const char *salt, size_t salt_sz) {
    blake2b_state BlakeHash;
    uint32_t value;

    blake2b_init(&BlakeHash, ARGON2_PREHASH_DIGEST_LENGTH);

    value = profile.thr_cost;
    blake2b_update(&BlakeHash, (const uint8_t *)&value, sizeof(value));

    value = ARGON2_RAW_LENGTH;
    blake2b_update(&BlakeHash, (const uint8_t *)&value, sizeof(value));

    value = profile.mem_cost;
    blake2b_update(&BlakeHash, (const uint8_t *)&value, sizeof(value));

    value = profile.tm_cost;
    blake2b_update(&BlakeHash, (const uint8_t *)&value, sizeof(value));

    value = ARGON2_VERSION;
    blake2b_update(&BlakeHash, (const uint8_t *)&value, sizeof(value));

    value = ARGON2_TYPE_VALUE;
    blake2b_update(&BlakeHash, (const uint8_t *)&value, sizeof(value));

    value = (uint32_t)base_sz;
    blake2b_update(&BlakeHash, (const uint8_t *)&value, sizeof(value));

    blake2b_update(&BlakeHash, (const uint8_t *)base,
                   base_sz);

    value = (uint32_t)salt_sz;
    blake2b_update(&BlakeHash, (const uint8_t *)&value, sizeof(value));

    blake2b_update(&BlakeHash, (const uint8_t *)salt,
                   salt_sz);

    value = 0;
    blake2b_update(&BlakeHash, (const uint8_t *)&value, sizeof(value));
    blake2b_update(&BlakeHash, (const uint8_t *)&value, sizeof(value));

    blake2b_final(&BlakeHash, blockhash, ARGON2_PREHASH_DIGEST_LENGTH);
}

void argon2::__fill_first_blocks(const argon2profile &profile, uint8_t *blockhash, int thread) {
    block *blocks = (block *)(__seed_memory + thread * __seed_memory_offset);

    size_t lane_length;
    if(__lane_length == -1) {
        lane_length = profile.mem_cost / profile.thr_cost;
    }
    else {
        lane_length = __lane_length;
    }

    for (uint32_t l = 0; l < profile.thr_cost; ++l) {
        *((uint32_t*)(blockhash + ARGON2_PREHASH_DIGEST_LENGTH)) = 0;
        *((uint32_t*)(blockhash + ARGON2_PREHASH_DIGEST_LENGTH + 4)) = l;

        blake2b_long((void *)(blocks + l * lane_length), ARGON2_BLOCK_SIZE, blockhash,
                     ARGON2_PREHASH_SEED_LENGTH);

        *((uint32_t*)(blockhash + ARGON2_PREHASH_DIGEST_LENGTH)) = 1;

        blake2b_long((void *)(blocks + l * lane_length + 1), ARGON2_BLOCK_SIZE, blockhash,
                     ARGON2_PREHASH_SEED_LENGTH);
    }
}

void argon2::set_seed_memory(uint8_t *memory) {
    __seed_memory = memory;
}

void argon2::set_seed_memory_offset(size_t offset) {
    __seed_memory_offset = offset;
}

void argon2::set_lane_length(int length) {
    if(length > 0)
        __lane_length = length;
}

bool argon2::__check_hash(uint8_t *hash, int hash_sz, uint8_t *hash_ceil, int hash_ceil_sz) {
    if (hash_sz < 32)
    {
        return false;
    }
    for (int i = 0; i < hash_sz; i++)
    {
        uint8_t cb = i < hash_ceil_sz ? hash_ceil[i] : (uint8_t)0xff;
        if (cb > hash[i]) return true;
        if (cb < hash[i]) return false;
    }
    return true;
}

