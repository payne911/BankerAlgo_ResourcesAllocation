//#define _XOPEN_SOURCE 700   /* So as to allow use of `fdopen` and `getline`.  */

#include "../common/common.h"
#include "server_thread.h"

#include <netinet/in.h>
#include <netdb.h>

#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <time.h>

enum { NUL = '\0' };

enum {
    /* Configuration constants.  */
    max_wait_time = 7, // todo: was 30
    server_backlog_size = 5
};

unsigned int server_socket_fd;

// Nombre de client enregistré.
int nb_registered_clients;


/* Variable du journal. */

// Nombre de requêtes acceptées immédiatement (ACK envoyé en réponse à REQ).
unsigned int count_accepted = 0;

// Nombre de requêtes acceptées après un délai (ACK après REQ, mais retardé).
unsigned int count_wait = 0;

// Nombre de requête erronées (ERR envoyé en réponse à REQ).
unsigned int count_invalid = 0;

// Nombre de clients qui se sont terminés correctement
// (ACK envoyé en réponse à CLO).
unsigned int count_dispatched = 0;

// Nombre total de requête (REQ) traités.
unsigned int request_processed = 0;

// Nombre de clients ayant envoyé le message CLO.
unsigned int clients_ended = 0;

// TODO: Ajouter vos structures de données partagées, ici.
/*  nbr_processus might be equivalent to `nb_registered_clients` */
int nbr_processus;  //       : n     : amount of processes
int nbr_types_res;  //       : m     : amount of types of resources
int *available;     // [j]   : m     : qty of res 'j' available
int **max;          // [i,j] : n x m : max qty of res 'j' used by 'i'
int **alloc;        // [i,j] : n x m : qty of res 'j' allocated to 'i'
int **needed;       // [i,j] : n x m : needed[i,j] = max[i,j] - alloc[i,j]

pthread_mutex_t lock;

void
st_init ()
{
    // Initialise le nombre de clients connecté.
    nb_registered_clients = 0;

    // TODO

    // Attend la connection d'un client et initialise les structures pour
    // l'algorithme du banquier.

    struct cmd_header_t test_data;
    char test[250];
    read_socket(server_socket_fd, &test, sizeof(test), 4 * 1000);
    printf("test: %s\n", test);
    // todo see : http://rosettacode.org/wiki/Banker%27s_algorithm#C
    // todo see : https://www.geeksforgeeks.org/program-bankers-algorithm-set-1-safety-algorithm/





    //nbr_processus = 0; <-- todo: currently considering it equivalent to `nb_registered_clients`
    nbr_types_res = 0;

    // pre-calculating the sizes to malloc
    size_t line_size = nbr_types_res * sizeof(int);
    size_t matrix_size = nb_registered_clients * sizeof(int);

    // todo: check malloc for failure || todo: maybe use `calloc` since init=0 ?
    available = malloc(line_size); //size_t matrix_size = nb_registered_clients * sizeof(available);
    max       = malloc(matrix_size);
    alloc     = malloc(matrix_size);
    needed    = malloc(matrix_size);

    printf("sizes: %zu  |  %zu\n", line_size, matrix_size);

    for (int i = 0; i < nbr_types_res; i++) {
        available[i] = 0;
    }

    for (int i = 0; i < nb_registered_clients; i++) {
        max[i]    = malloc(line_size);
        alloc[i]  = malloc(line_size);
        needed[i] = malloc(line_size);

        for (int j = 0; j < nbr_types_res; j++) {
            max[i][j]    = 0;
            alloc[i][j]  = 0;
            needed[i][j] = 0;
        }
    }


    // END TODO
}


//void computeNeeded(int need[P][R], int maxm[P][R], int allot[P][R]) {
//    for (int i = 0 ; i < P ; i++)
//        for (int j = 0 ; j < R ; j++)
//            // Need of instance = maxm instance - allocated instance
//            need[i][j] = maxm[i][j] - allot[i][j];
//}


void
st_process_requests (server_thread * st, int socket_fd)
{
    // TODO: Remplacer le contenu de cette fonction
    struct pollfd fds[1];
    fds->fd = socket_fd;
    fds->events = POLLIN;
    fds->revents = 0;

    while(true) {
        struct cmd_header_t header = { .nb_args = 0 };

        int len = read_socket(socket_fd, &header, sizeof(header), max_wait_time * 1000);
        if (len > 0) {
            if (len != sizeof(header.cmd) && len != sizeof(header)) {
                printf ("Thread %d received invalid command size=%d!\n", st->id, len);
                break;
            }
            printf("Thread %d received command=%d, nb_args=%d\n", st->id, header.cmd, header.nb_args);
            // dispatch of cmd void thunk(int sockfd, struct cmd_header* header);
        } else {
            if (len == 0) {
                fprintf(stderr, "Thread %d, connection timeout\n", st->id);
            }
            break;
        }
    }
    close(socket_fd);
}


void
st_signal ()
{
    // TODO: Remplacer le contenu de cette fonction

    /* Signale aux clients de se terminer. (Selon `server\main.c`) */



    // TODO end
}

void *
st_code (void *param)
{
    server_thread *st = (server_thread *) param;

    struct sockaddr_in thread_addr;
    socklen_t socket_len = sizeof (thread_addr);
    int thread_socket_fd = -1;
    int end_time = time (NULL) + max_wait_time;

    // Boucle jusqu'à ce que `accept` reçoive la première connection.
    while (thread_socket_fd < 0)
    {
        thread_socket_fd =
                accept (server_socket_fd, (struct sockaddr *) &thread_addr, &socket_len);

        if (time (NULL) >= end_time)
        {
            break;
        }
    }

    // Boucle de traitement des requêtes.
    while (accepting_connections)
    {
        if (!nb_registered_clients && time (NULL) >= end_time)
        {
            fprintf (stderr, "Time out on thread %d.\n", st->id);
            pthread_exit (NULL);
        }
        if (thread_socket_fd > 0)
        {
            st_process_requests (st, thread_socket_fd);
            close (thread_socket_fd);
            end_time = time (NULL) + max_wait_time;
        }
        thread_socket_fd =
                accept (server_socket_fd, (struct sockaddr *) &thread_addr, &socket_len);
    }
    return NULL;
}


//
// Ouvre un socket pour le serveur.
//
void
st_open_socket (int port_number)
{
    server_socket_fd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_socket_fd < 0)
        perror ("ERROR opening socket");

    /* todo: verify I could replace "SO_REUSEPORT" with "SO_REUSEADDR" */
    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
        perror("setsockopt()");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset (&serv_addr, 0, sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons (port_number);

    if (bind(server_socket_fd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0)
        perror ("ERROR on binding");

    listen (server_socket_fd, server_backlog_size);
}


//
// Affiche les données recueillies lors de l'exécution du serveur.
// La branche else ne doit PAS être modifiée.
//
void
st_print_results (FILE * fd, bool verbose)
{
    if (fd == NULL) fd = stdout;
    if (verbose)
    {
        fprintf (fd, "\n---- Résultat du serveur ----\n");
        fprintf (fd, "Requêtes acceptées: %d\n", count_accepted);
        fprintf (fd, "Requêtes : %d\n", count_wait);
        fprintf (fd, "Requêtes invalides: %d\n", count_invalid);
        fprintf (fd, "Clients : %d\n", count_dispatched);
        fprintf (fd, "Requêtes traitées: %d\n", request_processed);
    }
    else
    {
        fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_wait,
                 count_invalid, count_dispatched, request_processed);
    }
}
