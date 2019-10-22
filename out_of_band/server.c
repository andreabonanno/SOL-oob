#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <setjmp.h>
#include <limits.h>
#include "server.h"
#include "utils.h"

static uint id_arg;
static int pipe_fd;
static pthread_mutex_t pipe_lock;
static int listen_socket_fd, new_socket_fd;
static SA_UN server_addr;
static sigjmp_buf jmpbuf;

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Invalid arguments");
        return EXIT_FAILURE;
    }
    id_arg = atoi(argv[1]);
    idtoaddr(&server_addr, AF_UNIX, id_arg);
    pipe_fd = atoi(argv[2]);
    atexit(clean_exit);

    sigset_t sset;
    sigset_t oldset;
    //All signals are blocked.
    check(sigfillset(&sset), "Cannot create signal mask");
    check(pthread_sigmask(SIG_SETMASK, &sset, NULL), "Cannot set signal mask");
    struct sigaction sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = handler_sigterm;
    check(sigaction(SIGTERM, &sigact, NULL), "Cannot install SIGTERM handler");
    check(listen_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0), "Failed to create socket");
    check(bind(listen_socket_fd, (SA *) &server_addr, sizeof(server_addr)), "Failed to bind socket");
    check(listen(listen_socket_fd, SOMAXCONN), "Failed to listen on socket");
    fprintf(stdout, "SERVER %u ACTIVE\n", id_arg);
    fflush(stdout);

    ssize_t thread_count = 0;
    int loop_accept_flag = TRUE;
    pthread_t thread[MAX_CLIENTS_BUF];

    //If a SIGTERM is delivered execution flow returns here for an orderly termination.
    if (sigsetjmp(jmpbuf, TRUE)) {
        loop_accept_flag = FALSE;
        for (size_t i = 0; i < thread_count; ++i) {
            pthread_cancel(thread[i]);
        }
    }

    while (loop_accept_flag) {
        //SIGTERM can be delivered only during the blocking accept.
        check(sigdelset(&sset, SIGTERM), "Cannot remove SIGTERM from mask");
        check(pthread_sigmask(SIG_SETMASK, &sset, &oldset), "Cannot set signal mask");
        check(new_socket_fd = accept(listen_socket_fd, NULL, NULL), "Accept failed");
        check(pthread_sigmask(SIG_SETMASK, &oldset, &sset), "Cannot set signal mask");
        fprintf(stdout, "SERVER %u CONNECT FROM CLIENT\n", id_arg);
        fflush(stdout);
        int *pclient = malloc(sizeof(int));
        *pclient = new_socket_fd;
        pthread_create(&thread[thread_count], NULL, handle_connection, pclient);
        thread_count++;
    }
    for (size_t i = 0; i < thread_count; ++i) {
        pthread_join(thread[i], NULL);
    }
    return EXIT_SUCCESS;
}

static void *handle_connection(void *new_socket_fd_ptr) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    int client_socket = *(int *) new_socket_fd_ptr;
    free(new_socket_fd_ptr);
    uint64_t client_id_no;
    struct timespec timer_old;
    struct timespec timer_new;
    ulong time_gap;
    int res_read;

    thread_report *client_info = (thread_report *) malloc(sizeof(client_info));
    client_info->time_gap_min = ULONG_MAX;
    client_info->client_skt = client_socket;

    pthread_cleanup_push(thread_cleaner, client_info)
            check(clock_gettime(CLOCK_REALTIME, &timer_old), "Failed to get timestamp");
            do {
                //Threads can be cancelled only during a blocking read.
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
                check(res_read = read(client_socket, &client_id_no, MSG_LEN), "Read error");
                pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
                check(clock_gettime(CLOCK_REALTIME, &timer_new), "Failed to get timestamp");
                client_info->client_id = my_ntoh64(client_id_no);
                if (res_read > 0) {
                    time_gap = tmspcto_ms(&timer_new) - tmspcto_ms(&timer_old);
                    timer_old.tv_sec = timer_new.tv_sec;
                    timer_old.tv_nsec = timer_new.tv_nsec;
                    fprintf(stdout, "SERVER %u INCOMING FROM %lx @ %lu\n", id_arg, client_info->client_id, time_gap);
                    fflush(stdout);
                    //The minimum time interval between two client messages is used as an estimate of the secret.
                    if (time_gap > 0 && time_gap < client_info->time_gap_min) {
                        client_info->time_gap_min = time_gap;
                    }
                }
            } while (res_read != 0);
    pthread_cleanup_pop(1);
    pthread_exit(EXIT_SUCCESS);
}

//Handles sending the estimate to the supervisor.
static void thread_cleaner(void *arg) {
    thread_report *client_info = (thread_report *) arg;
    close(client_info->client_skt);
    //Prevents sending unuseful data to the supervisor (client sent one or zero message to this server).
    if (client_info->time_gap_min != 0 && client_info->client_id != 0) {
        fprintf(stdout, "SERVER %u CLOSING %lx ESTIMATE %lu\n", id_arg, client_info->client_id,
                client_info->time_gap_min);
        fflush(stdout);
        struct iovec tmp_buf[3];
        tmp_buf[0].iov_base = (uint *) &id_arg;
        tmp_buf[0].iov_len = sizeof(uint);
        tmp_buf[1].iov_base = (uint64_t *) &client_info->client_id;
        tmp_buf[1].iov_len = sizeof(uint64_t);
        tmp_buf[2].iov_base = &client_info->time_gap_min;
        tmp_buf[2].iov_len = sizeof(ulong);
        pthread_mutex_lock(&pipe_lock);
        check(writev(pipe_fd, (const struct iovec *) &tmp_buf, 3), "Writev error");
        pthread_mutex_unlock(&pipe_lock);
    }
    free(client_info);
}

/*Jumps back to the portion of code responsible for the delivery of a SIGTERM.
Handler can be invoked only by the main thread.
Other threads inherits the full mask as they are created.*/
static void handler_sigterm(int signum) {
    siglongjmp(jmpbuf, TRUE);
}

static void clean_exit(void) {
    fprintf(stdout, "SERVER %u CLOSING\n", id_arg);
    fflush(stdout);
    close(listen_socket_fd);
    close(pipe_fd);
    unlink(server_addr.sun_path);
}