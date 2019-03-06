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

ssize_t read_socket(int sockfd, void *buf, size_t obj_sz, int timeout);

#endif
//BEGIN 1 7382479
//ACK 1 7382479

//ACK 0

