#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

#include "common.h"
#include <semaphore.h>

/* Port TCP sur lequel le serveur attend des connections.  */
extern int port_number;

/* Nombre de requêtes que chaque client doit envoyer.  */
extern int num_request_per_client;

/* Nombre de resources différentes.  */
extern int num_resources;

/* Quantité disponible pour chaque resource.  */
extern int *provisioned_resources;


typedef struct client_thread client_thread;
struct client_thread
{
    unsigned int id;
    pthread_t pt_tid;
    pthread_attr_t pt_attr;
    /* Added those to be able to make sensical requests. */
    int* alloc;
    int* max;
};


void ct_init (client_thread *);
void ct_create_and_start (client_thread *);
void ct_wait_server ();
void ct_print_results (FILE *, bool);



/* Our own methods. */
void send_request (client_thread *, int); // modified and relocated
bool ct_init_server();
void setup_socket(int *, client_thread *);
void terminate_client(client_thread *);
void read_err(int, int, int);

#define CLOSURE(SOCKET, CLIENT) \
    close(SOCKET); \
    terminate_client(CLIENT); \
    pthread_exit (NULL)

#define KILL_COND(SOCKET, OUTPUT, LEN, COND) \
    int ret = read_socket((SOCKET),&OUTPUT,(LEN)*sizeof(int), READ_TIMEOUT); \
    if(ret > 0) { \
        /* Received the header. */ \
        printf("-->MAIN THREAD received:(cmd_type=%s | nb_args=%d)\n", \
               TO_ENUM(OUTPUT.cmd), OUTPUT.nb_args); \
        if(COND) { /* ERR */ \
            printf("»»»»»»»»»» Protocol expected another header.\n"); \
            close(socket_fd); \
            return false; \
        } \
    } else { \
        printf("=======read_error=======len:%d\n", ret);/*shouldn't happen*/ \
        close(socket_fd); \
        return false; \
    }


#endif // CLIENTTHREAD_H
