#include <stdlib.h>

#include "bloom_filter.h"

uint64_t calc_hash(const char* str, uint64_t modulus, uint64_t seed) {
    const static size_t P = 13;

    size_t hash = seed;
    while(*str) {
        hash = (hash * P + *(str++)) % modulus;
    }
    return hash;
}

void bloom_init(struct BloomFilter* bloom_filter, uint64_t set_size, hash_fn_t hash_fn, uint64_t hash_fn_count) {
    bloom_filter->set_size = set_size;
    bloom_filter->set      = (uint64_t *)calloc((set_size + sizeof(uint64_t) - 1) >> 6, sizeof(uint64_t));

    bloom_filter->hash_fn       = hash_fn;
    bloom_filter->hash_fn_count = hash_fn_count;
}

void bloom_destroy(struct BloomFilter* bloom_filter) {
    free(bloom_filter->set);

    bloom_filter->set_size = 0;
    bloom_filter->set      = NULL;

    bloom_filter->hash_fn       = NULL;
    bloom_filter->hash_fn_count = 0;
}

void bloom_insert(struct BloomFilter* bloom_filter, Key key) {
    for(uint64_t i = 0; i < bloom_filter->hash_fn_count; i++) {
        uint64_t hash = calc_hash(key, bloom_filter->set_size, i);
        bloom_filter->set[hash >> 6] |= ((uint64_t)1 << (hash % 64));
    }
}

bool bloom_check(struct BloomFilter* bloom_filter, Key key) {
    bool is_inserted = true;

    for(uint64_t i = 0; i < bloom_filter->hash_fn_count; i++) {
        uint64_t hash = calc_hash(key, bloom_filter->set_size, i);
        is_inserted  &= (bloom_filter->set[hash >> 6] >> (hash % 64));
    }
    return is_inserted;
}