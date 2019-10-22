#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/un.h>
#include "utils.h"

//Makes error-checking code leaner for functions that return -1 when failing.
int check(int expr, const char *errmsg) {
    if (expr == ERROR_VAL) {
        perror(errmsg);
        exit(EXIT_FAILURE);
    }
    return expr;
}

void swap(uint *a, uint *b) {
    intptr_t temp = *a;
    *a = *b;
    *b = temp;
}

void shuffle(uint arr[], int n, ushort seed) {
    srand(seed);
    for (size_t i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        swap(&arr[i], &arr[j]);
    }
}

//Instantiates the socket struct with the proper address based on server name.
void idtoaddr(struct sockaddr_un *addr, int domain, uint id) {
    char addrname[UNIX_PATH_MAX];
    sprintf(addrname, SERVERNAME_FORMAT, id);
    strncpy(addr->sun_path, addrname, UNIX_PATH_MAX);
    addr->sun_family = domain;
}

ulong tmspcto_ms(struct timespec *time) {
    return (ulong) time->tv_sec * 1e3 + time->tv_nsec * 1e-6;
}