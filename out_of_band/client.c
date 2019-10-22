#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <signal.h>
#include "utils.h"

uint *servers_id = NULL;
int *socket_fd = NULL;
SA_UN *servers_addr = NULL;
uint p_arg = 0;
uint64_t client_id;

static void clean_exit(void) {
    fprintf(stdout, "CLIENT %lx DONE\n", client_id);
    fflush(stdout);
    if (servers_id != NULL)
        free(servers_id);
    if (socket_fd != NULL)
        free(socket_fd);
    if (servers_addr != NULL)
        free(servers_addr);
    for (ssize_t i = 0; i < p_arg; i++) {
        close(socket_fd[i]);
    }
}

//The server may terminate before the client does and the next write from the client will generate a SIGPIPE.
static void handler_sigpipe(int signum) {
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {

    uint k_arg;
    uint w_arg;
    struct timespec timer_seeder;
    ushort seed;
    uint64_t id_half1;
    uint64_t id_half2;
    uint64_t client_id_no;
    uint secret;
    struct timespec timer_wait;
    struct sigaction pipeact;

    if (argc != 4) {
        fprintf(stderr, "Invalid arguments");
        return EXIT_FAILURE;
    }

    p_arg = atoi(argv[1]);
    k_arg = atoi(argv[2]);
    w_arg = atoi(argv[3]);

    if (p_arg < 1 || p_arg >= k_arg || w_arg <= 3 * p_arg) {
        perror("Invalid arguments");
        return EXIT_FAILURE;
    }

    memset(&pipeact, 0, sizeof(pipeact));
    pipeact.sa_handler = handler_sigpipe;
    check(sigaction(SIGPIPE, &pipeact, NULL), "Cannot install SIGPIPE handler");
    check(clock_gettime(CLOCK_REALTIME, &timer_seeder), "Failed get time");
    seed = timer_seeder.tv_nsec * getpid();

    //Two halves are combined to get a fully randomized uint64.
    seed48(&seed);
    id_half1 = lrand48();
    id_half2 = lrand48();
    client_id = id_half1 << 32u | id_half2;
    client_id_no = my_hton64(client_id);
    secret = lrand48() % SECRET_MAX;
    timer_wait.tv_sec = secret / 1000;
    timer_wait.tv_nsec = (secret % 1000) * 1e6;
    fprintf(stdout, "CLIENT %lx SECRET %u\n", client_id, secret);
    fflush(stdout);
    if (atexit(clean_exit) != 0) {
        perror("Cannot set exit function");
        return EXIT_FAILURE;
    }
    servers_id = (uint *) malloc(k_arg * sizeof(uint));
    for (size_t j = 0; j < k_arg; j++) {
        servers_id[j] = j + 1;
    }
    shuffle(servers_id, k_arg, seed);
    socket_fd = (int *) malloc(p_arg * sizeof(int));
    servers_addr = (SA_UN *) calloc(p_arg, sizeof(SA_UN));
    for (size_t i = 0; i < p_arg; i++) {
        idtoaddr(&servers_addr[i], AF_UNIX, servers_id[i]);
        check(socket_fd[i] = socket(AF_UNIX, SOCK_STREAM, 0), "Failed to create socket");
        check(connect(socket_fd[i], (SA *) &servers_addr[i], sizeof(SA_UN)), "Cannot connect to socket");
    }
    for (ssize_t j = 0; j < w_arg; j++) {
        uint index_rnd = rand() % p_arg;
        check(write(socket_fd[index_rnd], &client_id_no, MSG_LEN), "Write error");
        nanosleep(&timer_wait, NULL);
    }
    return EXIT_SUCCESS;
}

