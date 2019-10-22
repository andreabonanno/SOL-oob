#ifndef OUT_OF_BAND_HASH_H
#define OUT_OF_BAND_HASH_H

//Nearest prime to 2^64 * phi for Knuth's multiplicative hashing method.
#define PRIME_64 11400714819323198549ul

typedef struct ll_elem_t {
    uint64_t client_id;
    uint secret;
    uint *servers;
    size_t servers_count;
    struct ll_elem_t *next;
} ll_elem_t;

typedef struct ht_t {
    ssize_t max_servers;
    struct ll_elem_t **table;
    size_t size_bits;
    size_t lenght;
} ht_t;

typedef struct server_data_t {
    uint64_t client_id;
    uint server_id;
    uint secret;
} server_data_t;

int hash_insert(ht_t *ht, server_data_t *data);

void hash_init(ht_t *ht, size_t size, size_t max_servers);

void hash_free(ht_t *ht);

#endif