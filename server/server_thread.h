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
void st_process_requests (server_thread *, int);
void *st_code (void *);
void st_print_results (FILE *, bool);



/* Our own methods. todo: move to `server_thread.c` ? */
void setup_socket(int *);
void get_args(int socket_fd, cmd_header_t *header, int tmpId); // todo: last int just for debug
bool send_msg(int, char *, size_t);
bool send_err(int, char *);

// todo: remove this macro?
#define ERR_COND(COND, MSG) \
    if(COND) { \
        send_err(socket_fd, MSG); \
        break; \
    }

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
void prot_unknown   (int, bool *, int *, int);


/* Array of functions used to automatically call the good function on enums. */
typedef void my_fct_type(int, bool *, int *, int);
static my_fct_type *enum_func[NB_COMMANDS + 1] = {
        &prot_begin,
        &prot_conf,
        &prot_init,
        &prot_req,
        &prot_ack,
        &prot_wait,
        &prot_end,
        &prot_clo,
        &prot_err,
        &prot_unknown
};
#define FCT_ARR(NAME) \
    void NAME (int socket_fd, bool *success, int* args, int len)

#endif
