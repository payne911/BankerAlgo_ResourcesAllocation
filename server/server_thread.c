//#define _XOPEN_SOURCE 700   /* So as to allow use of `fdopen` and `getline`.  */

#include "server_thread.h"

#include <netdb.h>

#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/poll.h>

#include <time.h>

enum { NUL = '\0' };

enum {
    /* Configuration constants. */
    max_wait_time = 7, // todo: was 30
    server_backlog_size = 5
};

unsigned int server_socket_fd;

// Nombre de client enregistré.
int nb_registered_clients;


/* Variable du journal. */

// Nombre de requêtes acceptées immédiatement (ACK envoyé en réponse à REQ).
unsigned int count_accepted = 0;

// Nbr de requêtes acceptées après un délai (ACK après REQ, mais retardé).
unsigned int count_wait = 0;

// Nbr de requête erronées (ERR envoyé en réponse à REQ).
unsigned int count_invalid = 0;

// Nbr de clients qui se sont terminés correctement (ACK envoyé en réponse à CLO).
unsigned int count_dispatched = 0;

// Nbr total de requête (REQ) traités.
unsigned int request_processed = 0;

// Nbr de clients ayant envoyé le message CLO.
unsigned int clients_ended = 0;



// TODO: Ajouter vos structures de données partagées, ici.

int nbr_server_thread = 0;   // todo: utile?
int *prov_res;               // todo: utile?

// #proc = #_reg_clients     : n     : amount of processes
int nbr_types_res;  //       : m     : amount of types of resources
int *available;     // [j]   : m     : qty of res 'j' available
//int **max;        // [i,j] : n x m : max qty of res 'j' used by 'i'
//int **alloc;      // [i,j] : n x m : qty of res 'j' allocated to 'i'
//int **needed;     // [i,j] : n x m : needed[i,j] = max[i,j] - alloc[i,j]


typedef struct client { // todo: try to integrate this other approach instead?
    int id;
    int *alloc;      // qty of each res allocated
    int *max;        // max qty for each res
    int *needed;     // max[i]-alloc[i] todo: might not be necessary (can be calculated)
    // bool visited; // todo: not required?
} client;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;





void
st_init ()
{
    // Initialise le nombre de clients connecté.
    nb_registered_clients = 0;

    // TODO

    // Attend la connection d'un client et initialise les structures pour
    // l'algorithme du banquier.

//    accepting_connections = false;


    /* Trying to establish connection (collecting socket_fd). */
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int socket_fd = -1;
    while (socket_fd < 0) {
        socket_fd = accept(server_socket_fd, (struct sockaddr *)&addr, &len);
    }


    /* Collect information from socket. Expecting `BEGIN 1 RNG`. */
    INIT_HEAD_R(head_r);
    WAIT_FOR(&head_r, 2, head_r.cmd != BEGIN && head_r.nb_args != 1);
    printf("received cmd_r:(cmd_type=%s | nb_args=%d)\n", TO_ENUM(head_r.cmd), head_r.nb_args);

    int args_r[head_r.nb_args];
    WAIT_FOR(args_r, head_r.nb_args, true);
    PRINT_EXTRACTED("BEGIN 1", head_r.nb_args, args_r);


    /* Send confirmation (`ACK 1 RNG`). */
    INIT_HEAD_S(head_s, ACK, 1);
    send_header(socket_fd, &head_s, sizeof(cmd_header_t));

    int args_s[] = {args_r[0]};  // now contains RNG
    printf("sending RNG: %d  | received RNG: %d\n", args_s[0], args_r[0]);
    send_args(socket_fd, args_s, sizeof(args_s));


    /* Await `CONF` to set up the variables for the Banker-Algo. */
    INIT_HEAD_R(head_r2);
    WAIT_FOR(&head_r2, 2, head_r2.cmd != CONF && head_r2.nb_args < 1);
    printf("received cmd_r:(cmd_type=%s | nb_args=%d))\n", TO_ENUM(head_r2.cmd), head_r2.nb_args);

    int provs_r[head_r2.nb_args];
    WAIT_FOR(provs_r, head_r2.nb_args, true);
    PRINT_EXTRACTED("CONF", head_r2.nb_args, provs_r);




    /* Initializing the Banker-Algo's variables. */
    nbr_types_res = head_r2.nb_args;
    size_t tmp = nbr_types_res * sizeof(int); // reused size of malloc

    available = malloc(tmp); // todo: OOM
    prov_res  = malloc(tmp);
    for (int i = 0; i < nbr_types_res; i++) {
        available[i] = provs_r[i];
        prov_res[i]  = provs_r[i];
    }




    /* Send confirmation (`ACK 0`). */
    INIT_HEAD_S(head_s2, ACK, 0);
    send_header(socket_fd, &head_s2, sizeof(cmd_header_t));
    printf("sent `ACK 0`\n");


//    accepting_connections = true; // todo: maybe not here?
    close(socket_fd);

    printf("\n-=-=-=-=-\ndone initializing BANK ALGO vars\n-=-=-=-=-\n\n");


    // END TODO
}



void
st_process_requests (server_thread * st, int socket_fd)
{
    // TODO: Remplacer le contenu de cette fonction

    while(true) {
        struct cmd_header_t header = { .nb_args = 0 };

        int len = read_socket(socket_fd, &header, sizeof(header), max_wait_time * 1000);
        if (len > 0) {
            if (len != sizeof(header.cmd) && len != sizeof(header)) {
                printf ("Thread %d received invalid command size=%d!\n", st->id, len);
                break;
            }
            printf("\n\nThread %d received command=%s, nb_args=%d\n", st->id, TO_ENUM(header.cmd), header.nb_args);
            // dispatch of cmd void thunk(int sockfd, struct cmd_header* header);


            /* Now that we have the header, process the args. */
            get_args(socket_fd, &header, st->id);

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
    printf("entered st_signal()\n");

//    free(available); // todo: maybe useful

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

    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEPORT, &(int){ 1 }, sizeof(int)) < 0) {
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





/*
 *#################################################
 *#              ADDITIONAL METHODS               #
 *#################################################
 */

// todo see : http://rosettacode.org/wiki/Banker%27s_algorithm#C
// todo see : https://www.geeksforgeeks.org/program-bankers-algorithm-set-1-safety-algorithm/




/* todo: remove the `tmpId` param after debugging */
void get_args(int socket_fd, cmd_header_t *header, int tmpId) {
    printf("----get_args():  sockfd: %d | cmd: %s | nb_args: %d\n",
            socket_fd, TO_ENUM(header->cmd), header->nb_args);

    if(header->cmd > NB_COMMANDS) {      // if undefined cmd type    todo: test that
        printf(">>>>>>>>modified header for UNKNOWN<<<<<<<<\n");
        header->cmd = NB_COMMANDS + 1;   // assign the error function
    }
    int size_args = header->nb_args * sizeof(int);
    bool success = false;

    printf("|____cmd:%s size_args:%d\n", TO_ENUM(header->cmd), size_args);

    if(header->cmd == END && header->nb_args == 0) {
        /* todo: Edge-case: header is `END 0`. */
        printf("<°°°°°°°°°°>Thread %d received `END 0`\n", tmpId);
    } else {

        /* Extract the args now that we have the header! */
        int args[header->nb_args];
        int len = read_socket(socket_fd, args, size_args, max_wait_time * 1000);
        if (len > 0) {
            if (len != size_args) {
                printf("-get_args():  Thread %d received invalid command size=%d!\n", tmpId, len);
            }
            for(int i = 0; i < header->nb_args; i++) {
                printf("-get_args():  Thread %d received arg#%d: %d\n", tmpId, i, args[i]);
            }

            /* Using an array of functions to execute the proper function. */
            enum_func[header->cmd](socket_fd, &success, args, header->nb_args);
            printf("-get_args():  Thread %d success bool: %s\n", tmpId, success?"true":"false");

        } else {
            if (len == 0) {
                fprintf(stderr, "-get_args():  Thread %d, connection timeout\n", tmpId);
            }
        }
    }
}





/* Functions to treat each type of command's response individually. */
/* void NAME (int socket_fd, bool *success, int* args, int len)     */

FCT_ARR(prot_begin) {
    printf("new BEGIN\n");
    *success = false; // shouldn't happen at that point
}

FCT_ARR(prot_conf) {
    printf("new CONF\n");
    *success = false; // shouldn't happen at that point
}

FCT_ARR(prot_init) {
    printf("new INIT\n");

    /* New user is connecting. */
    pthread_mutex_lock(&mutex);
    nb_registered_clients++;
    printf("number of clients: %d\n", nb_registered_clients);
    pthread_mutex_unlock(&mutex);

    /* Send confirmation (`ACK 0`). */
    INIT_HEAD_S(head_s, ACK, 0);
    send_header(socket_fd, &head_s, sizeof(cmd_header_t));
    printf("sent `ACK 0`\n");

    *success = true;
}

FCT_ARR(prot_req) {
    printf("new REQ\n");

    // todo : BANKER + ACK/ERR

    /* Send confirmation (`ACK 0`). */
    INIT_HEAD_S(head_s, ACK, 0);
    send_header(socket_fd, &head_s, sizeof(cmd_header_t));
    printf("sent `ACK 0`\n");

    *success = true;
}

FCT_ARR(prot_ack) {
    printf("new ACK\n");
    *success = false; // shouldn't happen
}

FCT_ARR(prot_wait) {
    printf("new WAIT\n");
    *success = false; // shouldn't happen
}

FCT_ARR(prot_end) {
    printf("new END\n");
    *success = false;  // shouldn't happen at that point
}

FCT_ARR(prot_clo) {
    printf("new CLO\n");
    *success = true;
}

FCT_ARR(prot_err) {
    printf("new ERR\n");
    *success = false; // shouldn't happen
}

FCT_ARR(prot_nbcmd) {
    printf("new NB_COMMANDS\n");
    *success = false; // shouldn't happen
}

FCT_ARR(prot_unknown) {
    printf("new UNKNOWN\n");
    *success = false; // shouldn't happen
}



// todo (oli)
            /*=====================================*/
            /*||  Banker Algorithm implemention  ||*/
            /*=====================================*/


//void computeNeeded(int need[P][R], int maxm[P][R], int allot[P][R]) {
//    for (int i = 0 ; i < P ; i++)
//        for (int j = 0 ; j < R ; j++)
//            // Need of instance = maxm instance - allocated instance
//            need[i][j] = maxm[i][j] - allot[i][j];
//}
