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
    max_wait_time = 30,
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

typedef struct client { // todo (oli): try to integrate this or the approach
    int id;
    int *alloc;      // qty of each res allocated
    int *max;        // max qty for each res
    int *needed;     // max[i]-alloc[i] todo (oli): might not be necessary (can be calculated)
    // bool visited; // todo (oli): maybe not necessary? (remember this is multi-threaded)
} client;
client *clients_list;       // todo (oli): utile? (à toi de voir...)

// #proc = #_reg_clients     : n     : amount of processes
int nbr_types_res;  //       : m     : amount of types of resources
int *available;     // [j]   : m     : qty of res 'j' available
//int **max;        // [i,j] : n x m : max qty of res 'j' used by 'i' // todo (oli): this is the "other approach"
//int **alloc;      // [i,j] : n x m : qty of res 'j' allocated to 'i'
//int **needed;     // [i,j] : n x m : needed[i,j] = max[i,j] - alloc[i,j]





pthread_mutex_t mut_c_accepted   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_c_wait       = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_c_invalid    = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_c_dispatched = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_c_processed  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_c_ended      = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_c_registered = PTHREAD_MUTEX_INITIALIZER;




void
st_init ()
{
    // Initialise le nombre de clients connecté.
    nb_registered_clients = 0;

    // TODO

    // Attend la connection d'un client et initialise les structures pour
    // l'algorithme du banquier.


    /* Trying to establish connection (collecting socket_fd). */
    int socket_fd = -1;
    setup_socket(&socket_fd);



    /* Collect header from socket. Expecting `BEGIN 1`. */
    INIT_HEAD_R(head_r);
//    WAIT_FOR(&head_r, 2, head_r.cmd != BEGIN && head_r.nb_args != 1);
    while(true) {
        int ret = read_socket(socket_fd, &head_r, 2 * sizeof(int), READ_TIMEOUT);
        if(ret > 0) { // todo if (len != size_args)
            /* Received the header. */
            printf("received cmd_r:(cmd_type=%s | nb_args=%d)\n", TO_ENUM(head_r.cmd), head_r.nb_args);
            if(head_r.cmd == BEGIN && head_r.nb_args == 1) {
                break;
            } else {
                send_err(socket_fd, "»»»»»»»»»» Protocol expected another header.");
            }
        } else {
            printf("=======read_error=======len:%d\n", ret); // shouldn't happen
        }
    }

    /* Collecting RNG from socket. */
    int args_r[head_r.nb_args];
    while(true) {
        int ret = read_socket(socket_fd, &args_r, sizeof(int), READ_TIMEOUT);
        if(ret > 0) { // todo if (len != size_args)
            /* Received the args. */
            PRINT_EXTRACTED("BEGIN 1", head_r.nb_args, args_r);
            break;
        } else {
            printf("=======read_error=======len:%d\n", ret); // shouldn't happen
        }
    }



    /* Send confirmation (`ACK 1 RNG`). */
    INIT_HEAD_S(head_s, ACK, 1);
    send_header(socket_fd, &head_s, sizeof(cmd_header_t));

    int args_s[] = {args_r[0]};  // now contains RNG
    printf("sending RNG: %d  | received RNG: %d\n", args_s[0], args_r[0]);
    send_args(socket_fd, args_s, sizeof(args_s));



    /* Await `CONF` to set up the variables for the Banker-Algo. */
    INIT_HEAD_R(head_r2);
//    WAIT_FOR(&head_r2, 2, head_r2.cmd != CONF && head_r2.nb_args < 1);
    while(true) {
        int ret = read_socket(socket_fd, &head_r2, 2 * sizeof(int), READ_TIMEOUT);
        if(ret > 0) { // todo if (len != size_args)
            /* Received the header. */
            printf("received cmd_r:(cmd_type=%s | nb_args=%d))\n", TO_ENUM(head_r2.cmd), head_r2.nb_args);
            if(head_r2.cmd == CONF && head_r2.nb_args >= 1) {
                break;
            } else {
                send_err(socket_fd, "»»»»»»»»»» Protocol expected another header.");
            }
        } else {
            printf("=======read_error=======len:%d\n", ret); // shouldn't happen
        }
    }

    /* Collecting args of `CONF`. */
    int provs_r[head_r2.nb_args];
    while(true) {
        int ret = read_socket(socket_fd, &provs_r, head_r2.nb_args * sizeof(int), READ_TIMEOUT);
        if(ret > 0) { // todo if (len != size_args)
            /* Received the args. */
            PRINT_EXTRACTED("CONF", head_r2.nb_args, provs_r);
            break;
        } else {
            printf("=======read_error=======len:%d\n", ret); // shouldn't happen
        }
    }



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
    SEND_ACK(head_s2);

    close(socket_fd);

    printf("\n-=-=-=-=-\ndone initializing BANK ALGO vars\n-=-=-=-=-\n\n");


    // END TODO
}



void
st_process_requests (server_thread * st, int socket_fd)
{
    // TODO: Remplacer le contenu de cette fonction

    struct cmd_header_t header = { .nb_args = 0 };

    int len = read_socket(socket_fd, &header, sizeof(header), max_wait_time * 1000);
    if (len > 0) {
        if (len != sizeof(header.cmd) && len != sizeof(header)) {
            printf ("Thread %d received invalid command size=%d!\n", st->id, len);
        } else {
            printf("\n\nThread %d received command=%s, nb_args=%d\n",
                    st->id, TO_ENUM(header.cmd), header.nb_args);
            // dispatch of cmd void thunk(int sockfd, struct cmd_header* header);


            /* Now that we have the header, process the args. */
            get_args(socket_fd, &header, st->id);
        }
    } else {
        if (len == 0) {
            fprintf(stderr, "Thread %d, connection timeout\n", st->id);
        }
    }
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





            /*
             *#################################################
             *#              ADDITIONAL METHODS               #
             *#################################################
             */



void setup_socket(int *socket_fd) {
    /// Sets up the file descriptor of the socket, once connected.

    *socket_fd = -1; // safety measure

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    while (*socket_fd < 0) {
        *socket_fd = accept(server_socket_fd, (struct sockaddr *)&addr, &len);
    }
}


/* todo: remove the `tmpId` param after debugging */
void get_args(int socket_fd, cmd_header_t *header, int tmpId) {
    printf("----get_args():  sockfd: %d | cmd: %s | nb_args: %d\n",
            socket_fd, TO_ENUM(header->cmd), header->nb_args);


    int size_args = header->nb_args * sizeof(int);
    bool success = false;
    printf("|____cmd:%s size_args:%d\n", TO_ENUM(header->cmd), size_args);


    if(header->cmd >= NB_COMMANDS || header->cmd < BEGIN || header->nb_args < 0) {
        /* Undefined header. */
        printf(">>>>>>>>modified header for UNKNOWN<<<<<<<<\n");
        header->cmd = NB_COMMANDS + 1;   // assign the error function
        enum_func[header->cmd](socket_fd, &success, NULL, header->nb_args);
    } else {

        /* Header is well-defined. */
        if(header->nb_args == 0) {
            /* Edge-case: header has 0 arguments. */
            enum_func[header->cmd](socket_fd, &success, NULL, header->nb_args);
            printf("|__get_args():  Thread %d success bool on %s: %s\n",
                   tmpId, TO_ENUM(header->cmd), success?"true":"false");
        } else {

            /* Extract the args now that we have the header! */
            int args[header->nb_args];
            int len = read_socket(socket_fd, args, size_args, max_wait_time * 1000);
            if (len > 0) {
                if (len != size_args) {
                    printf("|__get_args():  Thread %d received %s invalid command size=%d!\n",
                           tmpId, TO_ENUM(header->cmd), len);
                }
                for(int i = 0; i < header->nb_args; i++) {
                    printf("|__get_args():  Thread %d received %s arg#%d: %d\n",
                           tmpId, TO_ENUM(header->cmd), i, args[i]);
                }

                /* Using an array of functions to execute the proper function. */
                enum_func[header->cmd](socket_fd, &success, args, header->nb_args);
                printf("|__get_args():  Thread %d success bool on %s: %s\n",
                       tmpId, TO_ENUM(header->cmd), success?"true":"false");

            } else {
                if (len == 0) {
                    fprintf(stderr, "|__get_args():  Thread %d, connection timeout on %s\n",
                            tmpId, TO_ENUM(header->cmd));
                }
            }
        }
    }
}

bool send_msg(int fd, char *msg, size_t len) {
    /// Send the message through the socket.
    /// see: https://stackoverflow.com/a/49395422/9768291

    printf("_____send_msg(): length= %zu | fd:%d | msg='%s'\n", len, fd, msg);

    while (len > 0) {
        ssize_t l = send(fd, msg, len, MSG_NOSIGNAL);

        if (l > 0) {
            msg += l;
            len  -= l;
        } else if (l == 0) {
            fprintf(stderr, "send_msg(): empty?\n");
            break;
        } else if (errno == EINTR) { // a signal occured before any data was transmitted
            continue;
        } else {
            perror("send()");
            break;
        }
    }

    return (len == 0);
}

bool send_err(int socket_fd, char *msg) {
    /// To send an error message (`ERR`).

    size_t len = strlen(msg);
    printf("......send_err: msg='%s' of size:%zu\n", msg, len);


    INIT_HEAD_S(head_s, ERR, len);
    send_header(socket_fd, &head_s, sizeof(cmd_header_t));
    send_msg(socket_fd, msg, len * sizeof(char));

    return true; // todo: adjust
}





/* Functions to treat each type of command's response individually.          */
/* Those functions are only reached after the initialisation of the server.  */
/* void NAME (int socket_fd, bool *success, int* args, int len)              */

FCT_ARR(prot_BEGIN) {
    printf("received new BEGIN\n");
    send_err(socket_fd, "»»»»»»»»» Server already initialized.");
    *success = false; // shouldn't happen at that point
}

FCT_ARR(prot_CONF) {
    printf("received new CONF\n");
    send_err(socket_fd, "»»»»»»»»» Resources already declared.");
    *success = false; // shouldn't happen at that point
}

FCT_ARR(prot_INIT) {
    printf("received new INIT\n");
    if(len != nbr_types_res+1) {
        send_err(socket_fd, "»»»»»»»»» `INIT` must refer to the right amount of resources.");
        *success = false;
    } else {

//        // todo: edge-case INIT called twice by same client?
//        if(false) {
//            send_err(socket_fd, "»»»»»»»»» `INIT` can only be called once per client.");
//            *success = false;
//            return;
//        }

        // todo: edge-case INIT with negative values?
//        if(false) {
//            send_err(socket_fd, "»»»»»»»»» `INIT` cannot contain negative integers.");
//            *success = false;
//            return;
//        }

        /* New user is connecting. */
        pthread_mutex_lock(&mut_c_registered);
        nb_registered_clients++;
        printf("number of clients: %d\n", nb_registered_clients);
        pthread_mutex_unlock(&mut_c_registered);

        // todo (oli): create new client (malloc?) | args is guaranteed to be well-formed (but might be illogical, i.e. negative?)
        client newClient;
        newClient.id = args[0];
        for(int i=0; i<nbr_types_res; i++) {
            newClient.max[i] = args[i+1];
        }


        /* Send confirmation (`ACK 0`). */
        SEND_ACK(head_s);

        *success = true;
    }
}

FCT_ARR(prot_REQ) {
    printf("received new REQ\n");

    pthread_mutex_lock(&mut_c_processed);
    request_processed++; // todo: does it go here? ambiguous definition...
    pthread_mutex_unlock(&mut_c_processed);

    if(len != nbr_types_res+1) {
        pthread_mutex_lock(&mut_c_invalid);
        count_invalid++;
        pthread_mutex_unlock(&mut_c_invalid);
        send_err(socket_fd, "»»»»»»»»» `REQ` must refer to the right amount of resources.");
        *success = false;
    } else {

//        // todo: edge-case verify the `tid` arg exists among declared clients?
//        if(false) {
//            send_err(socket_fd, "»»»»»»»»» `REQ` cannot be called on non-existent client.");
//            *success = false;
//        }

        int result;
        bankAlgo(&result, args, len);

        /* Sending appropriate response. */
        switch(result) {
            case ACK:
                pthread_mutex_lock(&mut_c_accepted);
                count_accepted++;
                pthread_mutex_unlock(&mut_c_accepted);
                SEND_ACK(head_sA);
                break;
            case WAIT:
                pthread_mutex_lock(&mut_c_wait);
                count_wait++;
                pthread_mutex_unlock(&mut_c_wait);
                TEST_WAIT(head_sW, 1);
                break;
            case ERR:
                pthread_mutex_lock(&mut_c_invalid);
                count_invalid++;
                pthread_mutex_unlock(&mut_c_invalid);
                send_err(socket_fd, "»»»»»»»»» illogical `REQ` request.\0");
                break;
            default:
                printf("\n\n=============error in Banker's Algo=============\n");
                break;
        }

        *success = true;
    }
}

FCT_ARR(prot_ACK) {
    printf("received new ACK\n");
    send_err(socket_fd, "»»»»»»»»» Clients shouldn't send this header.");
    *success = false; // shouldn't happen
}

FCT_ARR(prot_WAIT) {
    printf("received new WAIT\n");
    send_err(socket_fd, "»»»»»»»»» Clients shouldn't send this header.");
    *success = false; // shouldn't happen
}

FCT_ARR(prot_END) {
    printf("received new END\n");
    if(len != 0) {
        send_err(socket_fd, "»»»»»»»»» `END` can only declare 0 arguments.");
        *success = false;
    } else {

        // todo: not sure about this
        while(nb_registered_clients > 0);
        accepting_connections = false;

        /* Send confirmation (`ACK 0`). */
        SEND_ACK(head_s);

        // todo : close down the server (free all memory allocated)

        /* Freeing up the memory. */
        free(available);
        free(prov_res);

        *success = true;
    }
}

FCT_ARR(prot_CLO) {
    printf("received new CLO\n");
    pthread_mutex_lock(&mut_c_ended);
    clients_ended++; // todo: ambiguous definition, maybe not here?
    pthread_mutex_unlock(&mut_c_ended);

    if(len != 1) {
        send_err(socket_fd, "»»»»»»»»» `CLO` must have only 1 argument.");
        *success = false;
    } else {

        // todo: edge-case verify the `tid` arg exists among declared clients?

        // todo (oli): `free` banker's vars associated with client (probably should use mutex?)

        /* User is disconnecting. */
        pthread_mutex_lock(&mut_c_dispatched);
        count_dispatched++;
        pthread_mutex_unlock(&mut_c_dispatched);
        pthread_mutex_lock(&mut_c_registered);
        nb_registered_clients--;
        printf("number of clients: %d\n", nb_registered_clients);
        pthread_mutex_unlock(&mut_c_registered);

        /* Send confirmation (`ACK 0`). */
        SEND_ACK(head_s);

        *success = true;
    }
}

FCT_ARR(prot_ERR) {
    printf("received new ERR\n");
    send_err(socket_fd, "»»»»»»»»» Clients shouldn't send this header.");
    *success = false; // shouldn't happen
}

FCT_ARR(prot_UNKNOWN) {
    printf("received new UNKNOWN\n");
    send_err(socket_fd, "»»»»»»»»» Invalid header.");
    *success = false; // shouldn't happen
}





// todo (oli)
            /*=====================================*/
            /*||  Banker Algorithm implemention  ||*/
            /*=====================================*/

//void computeNeeded(int need[P][R], int max[P][R], int alloc[P][R]) {
//    for (int i = 0 ; i < P ; i++)
//        for (int j = 0 ; j < R ; j++)
//            need[i][j] = maxm[i][j] - alloc[i][j];
//}

void bankAlgo(int *result, int *args, int len) {
    /// `result` will contain the response that will be sent back to the client.
    /// `args` contains the request of the client, without the header.
    /// `args` is guaranteed to be well-formed, but not necessarily logical.
    /// `len` equals the amount of integers that `args` contains.
    /// `args` might be: { tid = 1, resA = 2, resB = -3, resC = 14 }, len = 4.

    int id = args[0];

    // todo (oli): faire l'algorithme du banquier ici
    // todo see : http://rosettacode.org/wiki/Banker%27s_algorithm#C
    // todo see : https://www.geeksforgeeks.org/program-bankers-algorithm-set-1-safety-algorithm/

    // todo (oli): `*result` depends on result from Banker Algo
    /* Just to introduce some randomness to test out stuff in the meantime. */
    if(rand()%5 == 1 && (id == 0 || id == 1)) {
        *result = WAIT;
    } else if(rand()%10 == 1) {
        *result = ERR;
    } else {
        *result = ACK;
    }
}