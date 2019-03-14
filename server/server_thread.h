#ifndef SERVER_THREAD_H
#define SERVER_THREAD_H

#include "common.h"

extern bool accepting_connections;

typedef struct server_thread server_thread;
struct server_thread
{
  unsigned int id;
  pthread_t pt_tid;
  pthread_attr_t pt_attr;
};

void st_open_socket (int port_number);
void st_init (void);
void st_process_request (server_thread *, int);
void st_signal (void);
void *st_code (void *);
//void st_create_and_start(st);
void st_print_results (FILE *, bool);



/* Our own methods. */
void process_request(int, cmd_header_t);
void get_args(int socket_fd, cmd_header_t *header, int tmpId); // todo: last int just for debug

// protocol functions once the clients are initialized on the server
void prot_begin     (int, bool *, int *, int);
void prot_conf      (int, bool *, int *, int);
void prot_init      (int, bool *, int *, int);
void prot_req       (int, bool *, int *, int);
void prot_ack       (int, bool *, int *, int);
void prot_wait      (int, bool *, int *, int);
void prot_end       (int, bool *, int *, int);
void prot_clo       (int, bool *, int *, int);
void prot_err       (int, bool *, int *, int);
void prot_nbcmd     (int, bool *, int *, int);
void prot_unknown   (int, bool *, int *, int);


/* Array of functions used to automatically call the good function on enums. */
static fct_type *enum_func[NB_COMMANDS + 2] = {
        &prot_begin,
        &prot_conf,
        &prot_init,
        &prot_req,
        &prot_ack,
        &prot_wait,
        &prot_end,
        &prot_clo,
        &prot_err,
        &prot_nbcmd,
        &prot_unknown
};

#endif
