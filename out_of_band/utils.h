#ifndef OUT_OF_BAND_UTILS_H
#define OUT_OF_BAND_UTILS_H

//Network/machine order conversion based on endianess checked at compile-time.
#if __BIG_ENDIAN__
#define my_hton64(x) (x)
#define my_ntoh64(x) (x)
#else
#define my_hton64(x) ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32)
#define my_ntoh64(x) ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32)
#endif

#define TRUE 1
#define FALSE 0
#define ERROR_VAL -1
#define UNIX_PATH_MAX 108
#define SERVERNAME_FORMAT "OOB-server-%u"
#define MSG_LEN 8
#define SECRET_MAX 3000

typedef struct sockaddr SA;
typedef struct sockaddr_un SA_UN;

int check(int expr, const char *errmsg);

void swap(uint *a, uint *b);

void shuffle(uint arr[], int n, ushort seed);

void idtoaddr(struct sockaddr_un *addr, int domain, uint id);

ulong tmspcto_ms(struct timespec *time);

#endif