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
void bankAlgo(int *, int *);


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

#define SET_UP_BANK_VARS \
    bool finish[nb_registered_clients]; \
    int  work  [nbr_types_res]; \
    int  max   [nbr_types_res][nb_registered_clients]; \
    int  alloc [nbr_types_res][nb_registered_clients]; \
    int  needed[nbr_types_res][nb_registered_clients]; \
    for(int i=0; i<nb_registered_clients; i++) { \
        work[i]   = available[i]; /* todo: useful? */ \
        finish[i] = false; \
        for(int j=0; j<nbr_types_res; j++) { \
            max   [i][j] = clients_list[i].max[j]; \
            alloc [i][j] = clients_list[i].alloc[j]; \
            needed[i][j] = max[i][j] - alloc[i][j]; \
        } \
    }

#endif
