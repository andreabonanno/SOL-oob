#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "hash.h"

//Knuth's multiplicative method.
static unsigned int hash(uint64_t key, size_t size_bits) {
    return (key * PRIME_64) >> (64 - size_bits);
}

static ll_elem_t *llist_insert(ll_elem_t *head, server_data_t *data, ssize_t max_servers) {
    ll_elem_t *curr = head;
    while (curr != NULL) {
        if (curr->client_id == data->client_id) {
            if (curr->secret > data->secret) {
                curr->secret = data->secret;
            }
            for (ssize_t i = 0; i < curr->servers_count; ++i) {
                if (curr->servers[i] == data->server_id)
                    return head;
            }
            curr->servers[curr->servers_count] = data->server_id;
            curr->servers_count++;
            return head;
        }
        curr = curr->next;
    }
    curr = (ll_elem_t *) calloc(1, sizeof(ll_elem_t));
    curr->next = head;
    curr->client_id = data->client_id;
    curr->secret = data->secret;
    curr->servers_count = 1;
    curr->servers = (uint *) calloc(max_servers, sizeof(uint));
    curr->servers[0] = data->server_id;
    return curr;
}

void hash_init(ht_t *ht, size_t size, size_t max_servers) {
    size_t len = pow(2.0, size);
    ht->size_bits = size;
    ht->table = (ll_elem_t **) calloc(len, sizeof(ll_elem_t *));
    ht->lenght = len;
    ht->max_servers = max_servers;
}

int hash_insert(ht_t *ht, server_data_t *data) {
    ssize_t position = hash(data->client_id, ht->size_bits);
    ht->table[position] = llist_insert(ht->table[position], data, ht->max_servers);
    return 1;
}

void hash_free(ht_t *ht) {
    for (size_t i = 0; i < ht->lenght; ++i) {
        ll_elem_t *curr = ht->table[i];
        while (curr != NULL) {
            free(curr->servers);
            curr = curr->next;
        }
    }
    free(ht->table);
    free(ht);
}