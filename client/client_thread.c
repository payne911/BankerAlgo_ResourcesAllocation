/* This `define` tells unistd to define usleep and random.  */
#define _XOPEN_SOURCE 500

#include "client_thread.h"

int port_number = -1;
int num_request_per_client = -1;
int num_resources = -1;
int *provisioned_resources = NULL;

// Variable d'initialisation des threads clients.
unsigned int count = 0;


/* Variable du journal. */

// Nombre de requête acceptée (ACK reçus en réponse à REQ)
unsigned int count_accepted = 0;

// Nbr de requête en attente (WAIT reçus en réponse à REQ)
unsigned int count_on_wait = 0;

// Nbr de requête refusée (ERR reçus en réponse à REQ)
unsigned int count_invalid = 0;

// Nbr de client qui se sont terminés correctement (ACK reçus en réponse à CLO)
unsigned int count_dispatched = 0;

// Nbr total de requêtes envoyées (REQ)
unsigned int request_sent = 0;


/* todo: ensure useful */
int *client_res_max   = NULL;
int *client_res_alloc = NULL;
sem_t sem_pthread_join;


pthread_mutex_t mut_c_count      = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_c_accepted   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_c_wait       = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_c_invalid    = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_c_dispatched = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_c_req        = PTHREAD_MUTEX_INITIALIZER;



void *
ct_code (void *param)
{
    int socket_fd = -1;
    client_thread *ct = (client_thread *) param;


    // TP2 TODO
    // Vous devez ici faire l'initialisation des petits clients (`INIT`).


    /* Connect to the server. */
    setup_socket(&socket_fd, ct);


    /* Declare the client's resources to the server.   (Send `INIT res+1`) */
    INIT_HEAD_S(head_s, INIT, num_resources+1);
    send_header(socket_fd, &head_s, sizeof(cmd_header_t));

    int args_s[num_resources+1];
    args_s[0] = ct->id;    // `tid` is the first argument
    for(int i = 0; i<num_resources; i++) {
        args_s[i+1] = rand()%provisioned_resources[i]; // max = borné par P_R
    }
    printf("id %d sending INIT %d\n", ct->id, head_s.nb_args);
    PRINT_EXTRACTED("INIT", head_s.nb_args, args_s);
    send_args(socket_fd, args_s, sizeof(args_s));



    // todo: wait for `ACK 0` (or `ERR` ?)
    /* Await `ACK 0`. */
    INIT_HEAD_R(head_r);
    WAIT_FOR(&head_r, 2, head_r.cmd != ACK && head_r.nb_args != 0);
    printf("--->id %d received:(cmd_type=%s | nb_args=%d))\n",
            ct->id, TO_ENUM(head_r.cmd), head_r.nb_args);

    close(socket_fd);

    // TP2 TODO:END

    for (unsigned int request_id = 0; request_id < num_request_per_client; request_id++)
    {

        // TP2 TODO
        // Vous devez ici coder, conjointement avec le corps de send request,
        // le protocole d'envoi de requête.

        socket_fd = -1;
        setup_socket(&socket_fd, ct);
        send_request (ct->id, request_id, socket_fd);
        close(socket_fd);

        /* After last request, trigger the `CLO`. */ // todo: legal to move below `for` ?
        if(request_id == num_request_per_client - 1) {
            printf("\n\nLast request of client %d is being sent\n", ct->id);
            terminate_client(ct);
        }

        // TP2 TODO:END

        /* Attendre un petit peu (0s-0.1s) pour simuler le calcul. */
        usleep (random () % (100 * 1000));
        /* struct timespec delay;
         * delay.tv_nsec = random () % (100 * 1000000);
         * delay.tv_sec = 0;
         * nanosleep (&delay, NULL); */
    }

    pthread_exit (NULL);
}


// Vous devez modifier cette fonction pour faire l'envoie des requêtes.
// Les ressources demandées par la requête doivent être choisies aléatoirement
// (sans dépasser le maximum pour le client). Elles peuvent être positives
// ou négatives.
// Assurez-vous que la dernière requête d'un client libère toute les ressources
// qu'il a jusqu'alors accumulées.
void
send_request (int client_id, int request_id, int socket_fd)
{

    // TP2 TODO

    fprintf (stdout, "\n\nClient %d is sending its %dth request through fd %d\n",
             client_id, request_id, socket_fd);


    /* Send resource request: `REQ`. */
    INIT_HEAD_S(head_s, REQ, num_resources+1);
    send_header(socket_fd, &head_s, sizeof(cmd_header_t));

    int args_s[num_resources+1];
    args_s[0] = client_id;       // `tid` is the first argument
    for(int i=0; i<num_resources; i++) {
        int rand_req = (rand()%(2*provisioned_resources[i]))-provisioned_resources[i];
        args_s[i+1] = rand_req;  // max = borné par P_R
    }
    printf("id %d sending REQ, arg0= %d\n", client_id, args_s[0]);
    PRINT_EXTRACTED("REQ", head_s.nb_args, args_s);
    send_args(socket_fd, args_s, sizeof(args_s));

    /* Increment stats. */
    pthread_mutex_lock(&mut_c_req);
    request_sent++;
    pthread_mutex_unlock(&mut_c_req);


    /* Await `ACK 0`. */        // todo could also be ERR or WAIT
    INIT_HEAD_R(head_r);
    WAIT_FOR(&head_r, 2, head_r.cmd != ACK && head_r.nb_args != 0);
    printf("-->id %d received head_r:(cmd_type=%s | nb_args=%d))\n",
           client_id, TO_ENUM(head_r.cmd), head_r.nb_args);

    /* Increment stats. */      // todo: diff replies : increase diff stats
    pthread_mutex_lock(&mut_c_accepted);
    count_accepted++;
    pthread_mutex_unlock(&mut_c_accepted);


    // TP2 TODO:END

}


//
// Vous devez changer le contenu de cette fonction afin de régler le
// problème de synchronisation de la terminaison.
// Le client doit attendre que le serveur termine le traitement de chacune
// de ses requêtes avant de terminer l'exécution.
//
void
ct_wait_server ()
{

    // TP2 TODO

    printf("Now waiting for all clients to `CLO`\n");
    sem_wait(&sem_pthread_join);
//    while(count > 0);
    printf("\n\n\n\n=========================================================\n"
           "Executing the `END` sequence.\n");

    /* Set up the socket. */
    int socket_fd = -1;
    setup_socket(&socket_fd, NULL);


    /* Terminate the server. */
    INIT_HEAD_S(head_s, END, 0);
    send_header(socket_fd, &head_s, sizeof(cmd_header_t));


    /* Await `ACK 0`. */        // todo could also be ERR or WAIT ?
    INIT_HEAD_R(head_r);
    WAIT_FOR(&head_r, 2, head_r.cmd != ACK && head_r.nb_args != 0);
    printf("-->main received head_r:(cmd_type=%s | nb_args=%d))\n",
           TO_ENUM(head_r.cmd), head_r.nb_args);


    sleep (4); // todo: eventually remove?

    close(socket_fd);

    // TP2 TODO:END

}


void
ct_init (client_thread * ct)
{
    ct->id = count++;
    sem_wait(&sem_pthread_join);
}

void
ct_create_and_start (client_thread * ct)
{
    pthread_attr_init (&(ct->pt_attr));
    pthread_attr_setdetachstate(&(ct->pt_attr), PTHREAD_CREATE_DETACHED);
    pthread_create (&(ct->pt_tid), &(ct->pt_attr), &ct_code, ct);
}

//
// Affiche les données recueillies lors de l'exécution du serveur.
// La branche else ne doit PAS être modifiée.
//
void
ct_print_results (FILE * fd, bool verbose)
{
    if (fd == NULL)
        fd = stdout;
    if (verbose)
    {
        fprintf (fd, "\n---- Résultat du client ----\n");
        fprintf (fd, "Requêtes acceptées: %d\n", count_accepted);
        fprintf (fd, "Requêtes : %d\n", count_on_wait);
        fprintf (fd, "Requêtes invalides: %d\n", count_invalid);
        fprintf (fd, "Clients : %d\n", count_dispatched);
        fprintf (fd, "Requêtes envoyées: %d\n", request_sent);
    }
    else
    {
        fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_on_wait,
                 count_invalid, count_dispatched, request_sent);
    }
}







/*
 *#################################################
 *#              ADDITIONAL METHODS               #
 *#################################################
 */


void ct_init_server(int num_clients) {
    /// Initializes the server properly before clients get "instanciated".

    /* Used for the ct_wait */
    sem_init(&sem_pthread_join, 0, num_clients);

    /* Set up the socket for the client. */
    int socket_fd = -1;
    setup_socket(&socket_fd, NULL);

    /* Initialize the server.   (Send `BEGIN 1 RNG`) */
    INIT_HEAD_S(head_s, BEGIN, 1);
    send_header(socket_fd, &head_s, sizeof(cmd_header_t));

    int args_s[] = { rand() }; // RNG
    printf("MAIN THREAD sending BEGIN 1 %d\n", args_s[0]);
    send_args(socket_fd, args_s, sizeof(args_s));




    // todo: wait for `ACK 1 RNG` (or `ERR` ?)
    /* Await `ACK 1 RNG`. */
    INIT_HEAD_R(head_r);
    WAIT_FOR(&head_r, 2, head_r.cmd != ACK && head_r.nb_args != 1);
    printf("received cmd_r:(cmd_type=%s | nb_args=%d))\n", TO_ENUM(head_r.cmd), head_r.nb_args);

    int args_r[head_r.nb_args];
    WAIT_FOR(args_r, head_r.nb_args, true); // todo: make sure 'true' is fine
    PRINT_EXTRACTED("ACK", head_r.nb_args, args_r);

    printf("MAIN THREAD received RNG: %d | had sent RNG: %d\n", args_r[0], args_s[0]);



    // todo: what does CONF expect as response ?
    /* Send `CONF` to set up the variables for the Banker-Algo. */
    printf("prov res: %d %d\n", provisioned_resources[0], provisioned_resources[1]);
    INIT_HEAD_S(head_s2, CONF, num_resources);
    send_header(socket_fd, &head_s2, sizeof(cmd_header_t));

    printf("MAIN THREAD sending CONF | num_res=%d\n", num_resources);
    PRINT_EXTRACTED("CONF", num_resources, provisioned_resources);
    send_args(socket_fd, provisioned_resources, num_resources*sizeof(int));



    // todo: wait for `ACK 0` (or `ERR` ?)
    /* Await `ACK 0`. */
    INIT_HEAD_R(head_r2);
    WAIT_FOR(&head_r2, 2, head_r2.cmd != ACK && head_r2.nb_args != 0);
    printf("-->MAIN THREAD received:(cmd_type=%s | nb_args=%d))\n",
           TO_ENUM(head_r2.cmd), head_r2.nb_args);


    /* This single-purpose client is over. */
    printf("\n-=-=-=-=-\ndone initializing BANK ALGO vars\n-=-=-=-=-\n\n");
    close(socket_fd);
}


void setup_socket(int *socket_fd, client_thread *ct) {
    /// Connects client to socket, and stores the proper variable for reuse.
    /// 'NULL' as the second argument means that it's the main thread.
    /// see: http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html

    *socket_fd = -1; // possibly redundant: just making sure

    struct sockaddr_in addr;
    *socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*socket_fd < 0) {
        perror("ct_code() | ERROR opening socket ");
        exit(-1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_number);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(*socket_fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("ct_code() | ERROR connecting ");
        exit(-1);
    } else {
        if(ct != NULL)
            printf("--Client %d connected on socket_fd: %d\n", ct->id, *socket_fd);
        else
            printf("--Client MAIN THREAD connected on socket_fd: %d\n", *socket_fd);
    }
}


void terminate_client(client_thread * client) {
    /// Announces the end of the client to the server.

    int socket_fd = -1;
    setup_socket(&socket_fd, client);
    int tid = client->id;

    /* Announce client's closure. */
    INIT_HEAD_S(head_s, CLO, 1);
    send_header(socket_fd, &head_s, sizeof(cmd_header_t));

    int args_s[] = {tid}; // `tid` is the only argument
    printf("id %d sending `CLO`, arg0= %d\n", tid, args_s[0]);
    PRINT_EXTRACTED("CLO", head_s.nb_args, args_s);
    send_args(socket_fd, args_s, sizeof(args_s));

    /* Increment stats. */
    pthread_mutex_lock(&mut_c_dispatched);
    count_dispatched++; // todo: not that!?
    pthread_mutex_unlock(&mut_c_dispatched);


    /* Await `ACK 0`. */        // todo could also be ERR or WAIT
    INIT_HEAD_R(head_r);
    WAIT_FOR(&head_r, 2, head_r.cmd != ACK && head_r.nb_args != 0);
    printf("-->id %d received head_r:(cmd_type=%s | nb_args=%d))\n",
           tid, TO_ENUM(head_r.cmd), head_r.nb_args);

    printf("Client id %d using fd %d should be terminated on server-side.\n",
            tid, socket_fd);

//    pthread_mutex_lock(&mutex);
//    count--;
//    pthread_mutex_unlock(&mutex);
    sem_post(&sem_pthread_join);

    close(socket_fd);
}


void ct_free(client_thread *ct) {
    /// Used to free up all the allocated memory.

    free(ct);
}