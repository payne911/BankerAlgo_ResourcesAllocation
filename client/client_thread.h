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
  // todo: banker's vars
};


void ct_init (client_thread *);
void ct_create_and_start (client_thread *);
void ct_wait_server ();
void ct_print_results (FILE *, bool);
void send_request (int, int, int); // added here to relocate



/* Our own methods.  todo: move to `client_thread.c` ? */
void ct_free(client_thread *);
void ct_init_server(int);
void setup_socket(int *, client_thread *);
void terminate_client(client_thread *);



#endif // CLIENTTHREAD_H
