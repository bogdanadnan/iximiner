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

argon2::argon2(argon2_blocks_filler_ptr filler, void *seed_memory, void *user_data) {
    __filler = filler;
    __threads = 1;
    __output_memory = __seed_memory = (uint8_t*)seed_memory;
    __seed_memory_offset = argon2profile_default->memsize;
    __lane_length = -1;
    __user_data = user_data;
}

vector<hash_data> argon2::generate_hashes(const argon2profile &profile, hash_data &input) {
    __inputs.clear();

    for(int i=0; i < __threads; i++) {
        input.nonce = __make_nonce();
        __inputs.push_back(input);
    }

    initialize_seeds(profile);
    fill_blocks(profile);
    encode_hashes(profile);

    return __inputs;
}

void argon2::initialize_seeds(const argon2profile &profile) {
    uint8_t blockhash[ARGON2_PREHASH_SEED_LENGTH];
    unsigned char base[256];
    unsigned char salt[256];
    size_t base_sz = 0;
    size_t salt_sz = 0;

    for(int i=0;i<__threads;i++) {
        base_sz = hex::decode(__inputs[i].base.c_str(), base, 256);
        salt_sz = hex::decode(__inputs[i].nonce.c_str(), salt, 256);

        __initial_hash(profile, blockhash, (char *)base, base_sz, (char *)salt, salt_sz);

        memset(blockhash + ARGON2_PREHASH_DIGEST_LENGTH, 0,
               ARGON2_PREHASH_SEED_LENGTH -
               ARGON2_PREHASH_DIGEST_LENGTH);

        __fill_first_blocks(profile, blockhash, i);
    }
}

void argon2::fill_blocks(const argon2profile &profile) {
    __output_memory = (uint8_t *)(*__filler) (__seed_memory, __threads, (argon2profile*)&profile, __user_data);
}

void argon2::encode_hashes(const argon2profile &profile) {
    unsigned char raw_hash[ARGON2_RAW_LENGTH];
    char encoded_hash[ARGON2_RAW_LENGTH * 2 + 1];

    if(__output_memory != NULL) {
        for (int i = 0; i < __threads; i++) {
            blake2b_long((void *) raw_hash, ARGON2_RAW_LENGTH,
                         (void *) (__output_memory + i * __seed_memory_offset), ARGON2_BLOCK_SIZE);


            hex::encode(raw_hash, ARGON2_RAW_LENGTH, encoded_hash);

            __inputs[i].hash = encoded_hash;
        }
    }
}

string argon2::__make_nonce() {
    unsigned char input[64];
    char output[129];

    random_generator::instance().get_random_data(input, 64);

    // DEBUG
//    memcpy(input, "1234567890 1234567890 1234567890 1234567890 1234567890 1234567890", 64);

    hex::encode(input, 64, output);
    return string(output);
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

uint8_t *argon2::get_output_memory() {
    return __output_memory;
}

void argon2::set_seed_memory_offset(size_t offset) {
    __seed_memory_offset = offset;
}

void argon2::set_threads(int threads) {
    __threads = threads;
}

void argon2::set_lane_length(int length) {
    if(length > 0)
        __lane_length = length;
}



