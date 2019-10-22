#ifndef OUT_OF_BAND_SUPERVISOR_H
#define OUT_OF_BAND_SUPERVISOR_H

#include "hash.h"

//Hash table with the size of 2^HASH_SIZE.
#define HASH_SIZE 8
//Delay for a SIGINT to be considered "double SIGINT"
#define SIGINT_DELAY_MS 1000

static void handler_sigint(int signum);

static void print_results(ht_t *table);

static void clean_exit(void);

#endif