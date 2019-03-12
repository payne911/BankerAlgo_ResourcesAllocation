#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

#include "../common/common.h" // todo: revert to common.h ?

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
};


void ct_init (client_thread *);
void ct_create_and_start (client_thread *);
void ct_wait_server ();
void ct_print_results (FILE *, bool);



// our own methods
void setup_socket();
void setup_frame(frame*, cmd_header_t*, int, int, int[]);
bool send_data(int*, frame*, size_t);

// macro-shortcuts for well-defined frames
#define mBEGIN BEGIN, 2, (int[]) {1,rand()}
#define mEND END, 0, NULL



#endif // CLIENTTHREAD_H
