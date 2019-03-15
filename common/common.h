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
    cmd_header_t NAME; \
    NAME.cmd = CMD_TYPE; \
    NAME.nb_args = NB_ARGS

#define TO_ENUM(x) \
    (x==BEGIN)?"`BEGIN`": \
    (x==CONF) ?"`CONF`": \
    (x==INIT) ?"`INIT`": \
    (x==REQ)  ?"`REQ`": \
    (x==ACK)  ?"`ACK`": \
    (x==WAIT) ?"`WAIT`": \
    (x==END)  ?"`END`": \
    (x==CLO)  ?"`CLO`": \
    (x==ERR)  ?"`ERR`": \
    (x==NB_COMMANDS)?"`NB_COMMANDS`": \
    "`UNKNOWN`"

#define TO_ENUM_NUM(x, y) \
    (x==BEGIN)?"`BEGIN` y": \
    (x==CONF) ?"`CONF` y": \
    (x==INIT) ?"`INIT` y": \
    (x==REQ)  ?"`REQ` y": \
    (x==ACK)  ?"`ACK` y": \
    (x==WAIT) ?"`WAIT` y": \
    (x==END)  ?"`END` y": \
    (x==CLO)  ?"`CLO` y": \
    (x==ERR)  ?"`ERR` y": \
    (x==NB_COMMANDS)?"`NB_COMMANDS` y": \
    "`UNKNOWN` y"

#define PRINT_EXTRACTED(NAME, FORVAR, VAR) \
    for(int i=0; i<FORVAR; i++) { \
        printf("-_=_-extracted from `%s` index %d = %d\n", (NAME), i, VAR[i]); \
    } \

#define WAIT_FOR(OUTPUT, LEN, COND) \
    while((COND)) { \
        int ret = read_socket(socket_fd, (OUTPUT), (LEN)*sizeof(int), READ_TIMEOUT); \
        if(ret > 0) { \
            /* received the object (todo: possibly only partly?) */ \
            break; \
        } else { \
            printf("=======e=bug========len=%d\n", ret); \
            break; \
        } \
    }

ssize_t read_socket(int sockfd, void *buf, size_t obj_sz, int timeout);

/* Our own methods. */
bool send_header(int, cmd_header_t *, size_t);
bool send_args  (int, int *, size_t);



#endif
//BEGIN 1 7382479
//ACK 1 7382479

//ACK 0

