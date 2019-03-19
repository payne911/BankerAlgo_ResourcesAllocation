#ifndef TP2_COMMON_H
#define TP2_COMMON_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//POSIX library for threads
#include <pthread.h>
#include <unistd.h>

#include <sys/types.h>
#include <poll.h>
#include <sys/socket.h>

//our own includes
#include <errno.h>
#include <netinet/in.h> // for `struct sockaddr_in`
#include <string.h>


enum cmd_type {
  BEGIN,
  CONF,
  INIT,
  REQ,
  ACK,// Mars Attack
  WAIT,
  END,
  CLO,
  ERR,
  NB_COMMANDS
};

typedef struct cmd_header_t {
    enum cmd_type cmd;
    int nb_args;
} cmd_header_t;



/* Our own macros. */
#define READ_TIMEOUT 4000 // 4 sec

#define INIT_HEAD_R(NAME) \
    cmd_header_t NAME; \
    NAME.cmd = -1; /* dummy */ \
    NAME.nb_args = -1

#define INIT_HEAD_S(NAME, CMD_TYPE, NB_ARGS) \
    cmd_header_t NAME = {.cmd=CMD_TYPE, .nb_args=NB_ARGS};

#define SEND_ACK(NAME) \
    cmd_header_t NAME = {.cmd=ACK, .nb_args=0}; \
    send_header(socket_fd, &NAME, sizeof(cmd_header_t)); \
    printf("sent `ACK 0`\n")

#define TEST_WAIT(NAME, TIME) \
    cmd_header_t NAME = {.cmd=WAIT, .nb_args=1}; \
    send_header(socket_fd, &NAME, sizeof(cmd_header_t)); \
    int args_s[] = { TIME }; \
    send_args(socket_fd, args_s, sizeof(args_s)); \
    printf("sent `WAIT 1`\n")

#define TO_ENUM(x) \
    (x==BEGIN)?"`BEGIN`": \
    (x==CONF) ?"`CONF`" : \
    (x==INIT) ?"`INIT`" : \
    (x==REQ)  ?"`REQ`"  : \
    (x==ACK)  ?"`ACK`"  : \
    (x==WAIT) ?"`WAIT`" : \
    (x==END)  ?"`END`"  : \
    (x==CLO)  ?"`CLO`"  : \
    (x==ERR)  ?"`ERR`"  : \
    "`UNKNOWN`"

#define PRINT_EXTRACTED(NAME, FORVAR, VAR) \
    for(int i=0; i<FORVAR; i++) { \
        printf("-_=_-extracted from `%s` index %d = %d\n", (NAME), i, VAR[i]); \
    }

#define WAIT_FOR(SOCKET, OUTPUT, LEN, COND) \
    while((COND)) { \
        int ret = read_socket(SOCKET, (OUTPUT), (LEN)*sizeof(int), READ_TIMEOUT); \
        if(ret > 0) { \
            /* received the object */ \
            break; \
        } else { \
            printf("=======e=bug========len=%d\n", ret); \
            break; \
        } \
    }

#define KILL_COND(SOCKET, OUTPUT, LEN, COND) \
    int ret = read_socket((SOCKET),&OUTPUT,(LEN)*sizeof(int), READ_TIMEOUT); \
    if(ret > 0) { /* todo if (len != size_args) */ \
        /* Received the header. */ \
        printf("-->MAIN THREAD received:(cmd_type=%s | nb_args=%d)\n", \
               TO_ENUM(OUTPUT.cmd), OUTPUT.nb_args); \
        if(COND) { /* ERR */ \
            printf("»»»»»»»»»» Protocol expected another header.\n"); \
            close(socket_fd); \
            return false; \
        } \
    } else { \
        printf("=======read_error=======len:%d\n", ret);/* shouldn't happen */ \
        close(socket_fd); \
        return false; \
    }


ssize_t read_socket(int sockfd, void *buf, size_t obj_sz, int timeout);

/* Our own methods. */
bool send_header(int, cmd_header_t *, size_t);
bool send_args  (int, int *, size_t);



#endif
//BEGIN 1 7382479
//ACK 1 7382479

//ACK 0

