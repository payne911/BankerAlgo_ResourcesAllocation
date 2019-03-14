/* This `define` tells unistd to define usleep and random.  */
#define _XOPEN_SOURCE 500

#include "client_thread.h"

int port_number = -1;
int num_request_per_client = -1;
int num_resources = -1;
int *provisioned_resources = NULL;

// Variable d'initialisation des threads clients.
unsigned int count = 0;


// Variable du journal.
// Nombre de requête acceptée (ACK reçus en réponse à REQ)
unsigned int count_accepted = 0;

// Nbr de requête en attente (WAIT reçus en réponse à REQ)
unsigned int count_on_wait = 0;

// Nbr de requête refusée (REFUSE reçus en réponse à REQ)
unsigned int count_invalid = 0;

// Nbr de client qui se sont terminés correctement (ACK reçus en réponse à END)
unsigned int count_dispatched = 0;

// Nbr total de requêtes envoyées.
unsigned int request_sent = 0;


/* todo: ensure useful */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
bool initialized_server = false;
int *client_res_max   = NULL;
int *client_res_alloc = NULL;


void *
ct_code (void *param)
{
    int socket_fd = -1;
    client_thread *ct = (client_thread *) param;


    // TP2 TODO
    // Vous devez ici faire l'initialisation des petits clients (`INIT`).

    /* Let ONE client initialize the server. */
    pthread_mutex_lock(&mutex);
    if(!initialized_server) {

        /* Set up the socket for the client. */
        setup_socket(&socket_fd, ct);

        /* Initialize the server.   (Send `BEGIN 1 RNG`) */
        INIT_HEAD_S(head_s, BEGIN, 1);
        send_header(socket_fd, &head_s, sizeof(cmd_header_t));

        int args_s[] = { rand() }; // RNG
        printf("id %d sending BEGIN 1 %d\n", ct->id, args_s[0]);
        send_args(socket_fd, args_s, sizeof(args_s));




        // todo: wait for `ACK 1 RNG` (or `ERR` ?)
        /* Await `ACK 1 RNG`. */
        INIT_HEAD_R(head_r);
        WAIT_FOR(&head_r, 2, head_r.cmd != ACK && head_r.nb_args != 1);
        printf("received cmd_r:(cmd_type=%s | nb_args=%d))\n", TO_ENUM(head_r.cmd), head_r.nb_args);

        int args_r[head_r.nb_args];
        WAIT_FOR(args_r, head_r.nb_args, true); // todo: make sure 'true' is fine
        PRINT_EXTRACTED("ACK", head_r.nb_args, args_r);

        printf("id %d received RNG: %d | had sent RNG: %d\n", ct->id, args_r[0], args_s[0]);



        // todo: what does CONF expect as response ?
        /* Send `CONF` to set up the variables for the Banker-Algo. */
        printf("prov res: %d %d\n", provisioned_resources[0], provisioned_resources[1]);
        INIT_HEAD_S(head_s2, CONF, num_resources);
        send_header(socket_fd, &head_s2, sizeof(cmd_header_t));

        printf("client %d sending CONF | num_res=%d\n", ct->id, num_resources);
        PRINT_EXTRACTED("CONF", num_resources, provisioned_resources);
        send_args(socket_fd, provisioned_resources, num_resources*sizeof(int));



        // todo: wait for `ACK 0` (or `ERR` ?)
        /* Await `ACK 0`. */
        INIT_HEAD_R(head_r2);
        WAIT_FOR(&head_r2, 2, head_r2.cmd != ACK && head_r2.nb_args != 0);
        printf("-->id %d received:(cmd_type=%s | nb_args=%d))\n",
               ct->id, TO_ENUM(head_r2.cmd), head_r2.nb_args);


        /* This single-purpose client is over. */
        printf("\n-=-=-=-=-\ndone initializing BANK ALGO vars\n-=-=-=-=-\n\n");
        initialized_server = true;
        close(socket_fd);
    }
    pthread_mutex_unlock(&mutex);


    /* Now any client may connect to the server. */
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


    // TP2 TODO:END

    for (unsigned int request_id = 0; request_id < num_request_per_client; request_id++)
    {

        // TP2 TODO
        // Vous devez ici coder, conjointement avec le corps de send request,
        // le protocole d'envoi de requête.

        send_request (ct->id, request_id, socket_fd);
        // todo: add here an if to check if last method and free memory?

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

    fprintf (stdout, "\n\nClient %d is sending its %d request through fd %d\n",
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
    pthread_mutex_lock(&mutex);
    request_sent++;
    pthread_mutex_unlock(&mutex);


    /* Await `ACK 0`. */        // todo could also be ERR or WAIT
    INIT_HEAD_R(head_r);
    WAIT_FOR(&head_r, 2, head_r.cmd != ACK && head_r.nb_args != 0);
    printf("-->id %d received head_r:(cmd_type=%s | nb_args=%d))\n",
           client_id, TO_ENUM(head_r.cmd), head_r.nb_args);

    /* Increment stats. */      // todo: diff replies : increase diff stats
    pthread_mutex_lock(&mutex);
    count_accepted++;
    pthread_mutex_unlock(&mutex);


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

    printf("entered ct_wait_server()\n");

    /* todo: Send resource request: `END`? */

    /* todo: increment count when successful? */
    /* Increment stats. */
//    pthread_mutex_lock(&mutex);
//    count_dispatched++;
//    pthread_mutex_unlock(&mutex);


    sleep (4);

    // TP2 TODO:END

}


void
ct_init (client_thread * ct)
{
    ct->id = count++;
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



void setup_socket(int *socket_fd, client_thread *ct) {
    /// Connects client to socket, and stores the proper variable for reuse.
    /// see: http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html

    struct sockaddr_in addr;
    *socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*socket_fd < 0) {
        perror("ct_code() | ERROR opening socket ");
        exit(-1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_number);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (connect(*socket_fd, &addr, sizeof(addr)) < 0) {
        perror("ct_code() | ERROR connecting ");
        exit(-1);
    } else {
        printf("--Client %d connected on socket_fd: %d\n", ct->id, *socket_fd);
    }
}
