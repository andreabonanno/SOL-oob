#ifndef OUT_OF_BAND_SERVER_H
#define OUT_OF_BAND_SERVER_H
#define MAX_CLIENTS_BUF 1024

typedef struct {
    uint64_t client_id;
    ulong time_gap_min;
    int client_skt;
} thread_report;

static void clean_exit(void);

static void handler_sigterm(int signum);

static void *handle_connection(void *new_socket_fd_ptr);

static void thread_cleaner(void *arg);

#endif
