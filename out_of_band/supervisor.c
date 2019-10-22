#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include "supervisor.h"
#include "utils.h"
#include "hash.h"

static sigjmp_buf jmpbuf;
static ht_t *table = NULL;
static pid_t *pid_childs = NULL;
static int **pipes = NULL;
static ssize_t server_count;

int main(int argc, char **argv) {

    pid_t pid_fork;
    char arg_buf[BUFSIZ];
    char arg2_buf[BUFSIZ];
    int fd_max = 0;
    int sigint_firsttime = TRUE;
    int loop_flag = TRUE;
    struct timespec last_sigint;

    fd_set set, rdset;
    sigset_t sset_full;
    sigset_t sset_sigint;
    check(sigfillset(&sset_full), "Cannot create signal mask");
    check(sigfillset(&sset_sigint), "Cannot create signal mask");
    check(sigdelset(&sset_sigint, SIGINT), "Cannot create SIGINT mask");
    //All signal are blocked.
    check(pthread_sigmask(SIG_SETMASK, &sset_full, NULL), "Cannot set signal mask");

    if (argc != 2) {
        fprintf(stderr, "Invalid arguments\n");
        return EXIT_FAILURE;
    }

    server_count = atoi(argv[1]);
    atexit(clean_exit);
    pid_childs = (pid_t *) malloc(server_count * sizeof(pid_t));
    pipes = (int **) malloc(server_count * sizeof(int *));
    for (ssize_t i = 0; i < server_count; ++i) {
        pipes[i] = (int *) malloc(2 * sizeof(int));
        pipe(pipes[i]);
    }
    fprintf(stdout, "SUPERVISOR STARTING %zd\n", server_count);
    fflush(stdout);
    FD_ZERO(&set);
    for (ssize_t k = 0; k < server_count; k++) {
        check(pid_fork = fork(), "Fork error");
        if (pid_fork == 0) {
            close(pipes[k][0]);
            sprintf(arg_buf, "%ld", k + 1);
            sprintf(arg2_buf, "%i", pipes[k][1]);
            execl("./server", "server", arg_buf, arg2_buf, (char *) NULL);
        }
        pid_childs[k] = pid_fork;
        close(pipes[k][1]);
        FD_SET(pipes[k][0], &set);
        if (pipes[k][0] > fd_max)
            fd_max = pipes[k][0];
    }

    table = (ht_t *) calloc(1, sizeof(ht_t));
    hash_init(table, HASH_SIZE, server_count);
    struct sigaction sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = handler_sigint;
    check(sigaction(SIGINT, &sigact, NULL), "Cannot install SIGINT handler");

    //If a SIGINT is delivered execution flow returns here for printing current estimates and eventually termination.
    if (sigsetjmp(jmpbuf, TRUE)) {
        if (sigint_firsttime) {
            clock_gettime(CLOCK_REALTIME, &last_sigint);
            sigint_firsttime = FALSE;
        } else {
            struct timespec new_sigint;
            ulong timegap;
            clock_gettime(CLOCK_REALTIME, &new_sigint);
            timegap = tmspcto_ms(&new_sigint) - tmspcto_ms(&last_sigint);
            last_sigint.tv_sec = new_sigint.tv_sec;
            last_sigint.tv_nsec = new_sigint.tv_nsec;
            if (timegap < SIGINT_DELAY_MS)
                loop_flag = FALSE;
        }
        print_results(table);
    }

    //Loop stops only if a double SIGINT is delivered
    while (loop_flag) {
        rdset = set;
        //Pselect allows SIGINT do be delivered while the call is blocking the execution flow.
        check(pselect(fd_max + 1, &rdset, NULL, NULL, NULL, &sset_sigint), "Select error");
        for (size_t fd = 0; fd <= fd_max; ++fd) {
            if (FD_ISSET(fd, &rdset)) {
                server_data_t temp_data;
                struct iovec tmp_buf[3];
                tmp_buf[0].iov_base = (uint *) &temp_data.server_id;
                tmp_buf[0].iov_len = sizeof(uint);
                tmp_buf[1].iov_base = (uint64_t *) &temp_data.client_id;
                tmp_buf[1].iov_len = sizeof(uint64_t);
                tmp_buf[2].iov_base = (ulong *) &temp_data.secret;
                tmp_buf[2].iov_len = sizeof(ulong);
                check(readv(fd, (const struct iovec *) &tmp_buf, 3), "Readv error");
                fprintf(stdout, "SUPERVISOR ESTIMATE %u FOR %lx FROM %u\n", temp_data.secret, temp_data.client_id,
                        temp_data.server_id);
                fflush(stdout);
                hash_insert(table, &temp_data);
            }
        }

    }

    //Termination of all servers is handled by the supervisor after the delivery of a double SINGINT.
    for (ssize_t j = 0; j < server_count; j++) {
        kill(pid_childs[j], SIGTERM);
    }
    for (size_t l = 0; l < server_count; ++l) {
        wait(NULL);
    }
    return EXIT_SUCCESS;
}

//Jumps back to the portion of code responsible for handling the delivery of a SIGINT.
static void handler_sigint(int signum) {
    siglongjmp(jmpbuf, TRUE);
}

static void print_results(ht_t *tab) {
    ll_elem_t *curr;
    for (size_t i = 0; i < table->lenght; ++i) {
        curr = tab->table[i];
        while (curr != NULL) {
            fprintf(stdout, "SUPERVISOR ESTIMATE %u FOR %lx BASED ON %zu\n", curr->secret, curr->client_id,
                    curr->servers_count);
            fflush(stdout);
            curr = curr->next;
        }
    }
}

static void clean_exit(void) {
    if (table != NULL)
        hash_free(table);
    if (pipes != NULL)
        for (int i = 0; i < server_count; ++i) {
            free(pipes[i]);
        }
    free(pipes);
    free(pid_childs);
}