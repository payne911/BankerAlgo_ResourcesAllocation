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



/* Our own methods. */
void setup_socket(int *);
void get_args(int, cmd_header_t *, int);
bool send_msg(int, char *, size_t);
bool send_err(int, char *);
void bankAlgo(int *, int *, int);


// protocol functions once the clients are initialized on the server
#define SIGNATURE(W,X,Y,Z) int W, bool *X, int *Y, int Z
void prot_BEGIN   (SIGNATURE(,,,));
void prot_CONF    (SIGNATURE(,,,));
void prot_INIT    (SIGNATURE(,,,));
void prot_REQ     (SIGNATURE(,,,));
void prot_ACK     (SIGNATURE(,,,));
void prot_WAIT    (SIGNATURE(,,,));
void prot_END     (SIGNATURE(,,,));
void prot_CLO     (SIGNATURE(,,,));
void prot_ERR     (SIGNATURE(,,,));
void prot_UNKNOWN (SIGNATURE(,,,));


/* Array of functions used to automatically call the good function on enums. */
typedef void my_fct_type(SIGNATURE(,,,));
static my_fct_type *enum_func[NB_COMMANDS + 1] = {
        &prot_BEGIN,
        &prot_CONF,
        &prot_INIT,
        &prot_REQ,
        &prot_ACK,
        &prot_WAIT,
        &prot_END,
        &prot_CLO,
        &prot_ERR,
        &prot_UNKNOWN
};
#define FCT_ARR(NAME) \
    void NAME (SIGNATURE(socket_fd, success, args, len))



#endif
