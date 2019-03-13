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

// Nombre de requête en attente (WAIT reçus en réponse à REQ)
unsigned int count_on_wait = 0;

// Nombre de requête refusée (REFUSE reçus en réponse à REQ)
unsigned int count_invalid = 0;

// Nombre de client qui se sont terminés correctement (ACC reçu en réponse à END)
unsigned int count_dispatched = 0;

// Nombre total de requêtes envoyées.
unsigned int request_sent = 0;

/* todo: ensure useful */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
bool initialized_server = false;
int *client_res_max   = NULL;
int *client_res_alloc = NULL;


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

    fprintf (stdout, "Client %d is sending its %d request through fd %d\n",
            client_id, request_id, socket_fd);



    cmd_header_t head_s;
    head_s.cmd = REQ;  // todo: implement the proper request
    head_s.nb_args = 6;
    send_header(socket_fd, &head_s, sizeof(cmd_header_t));

    int args_s[] = {8,3,2,5,6,2425}; // RNG
    printf("id %d sending REQ, arg0= %d\n", client_id, args_s[0]);
    send_args(socket_fd, args_s, sizeof(args_s));



    // TP2 TODO:END

}

void *
ct_code (void *param)
{
    int socket_fd = -1;
    client_thread *ct = (client_thread *) param;


    // TP2 TODO
    // Vous devez ici faire l'initialisation des petits clients (`INI`).

    /* Set up the socket for the client. */
    setup_socket(&socket_fd, ct);

    /* Let one client initialize the server. */
    pthread_mutex_lock(&mutex);
    if(!initialized_server) {

        /* Initialize the server.   (Send `BEGIN 1 RNG`) */
        cmd_header_t head_s;
        head_s.cmd = BEGIN;
        head_s.nb_args = 1;
        send_header(socket_fd, &head_s, sizeof(cmd_header_t));

        int args_s[] = { rand() }; // RNG
        printf("id %d sending BEGIN 1 %d\n", ct->id, args_s[0]);
        send_args(socket_fd, args_s, sizeof(args_s));




        // todo: wait for `ACK 1 RNG` (or `ERR` ?)
        /* Await `ACK 1 RNG`. */
        cmd_header_t head_r;
        head_r.nb_args = -1;
        WAIT_FOR(&head_r, 2, head_r.cmd != ACK && head_r.nb_args != 1);
        printf("received cmd_r:(cmd_type=%s | nb_args=%d))\n", TO_ENUM(head_r.cmd), head_r.nb_args);

        int args_r[head_r.nb_args];
        WAIT_FOR(args_r, head_r.nb_args, true); // todo: make sure 'true' is fine
        PRINT_EXTRACTED("ACK", head_r.nb_args, args_r);

        printf("id %d received RNG: %d | had sent RNG: %d\n", ct->id, args_r[0], args_s[0]);





        // todo: what does CONF expect as response ?
        /* Send `CONF` to set up the variables for the Banker-Algo. */
        printf("prov res: %d %d\n", provisioned_resources[0], provisioned_resources[1]);
        head_s.cmd = CONF;
        head_s.nb_args = num_resources;
        send_header(socket_fd, &head_s, sizeof(cmd_header_t));

        printf("client %d sending CONF | num_res=%d\n", ct->id, num_resources);
        PRINT_EXTRACTED("CONF", num_resources, provisioned_resources);
        send_args(socket_fd, provisioned_resources, sizeof(provisioned_resources));


        initialized_server = true;
    }
    pthread_mutex_unlock(&mutex);



    /* Declare the client's resources to the server.   (Send `INIT res+1`) */
    cmd_header_t head_s2;
    head_s2.cmd = INIT;
    head_s2.nb_args = num_resources+1;
    send_header(socket_fd, &head_s2, sizeof(cmd_header_t));

    int args_s2[] = {ct->id /*todo max_res*/, 5, 4, 2, 0, 1};
    printf("id %d sending INIT %d\n", ct->id, head_s2.nb_args);
    PRINT_EXTRACTED("INIT", head_s2.nb_args, args_s2);
    send_args(socket_fd, args_s2, sizeof(args_s2));

    // todo: wait for `ACK 0` (or `ERR` ?)
    /* Await `ACK 0`. */
    cmd_header_t head_r2;
    head_r2.cmd = REQ; // dummy
    head_r2.nb_args = -1;
    WAIT_FOR(&head_r2, 2, head_r2.cmd != ACK && head_r2.nb_args != 0);
    printf("-->id %d received cmd_r:(cmd_type=%s | nb_args=%d))\n",
            ct->id, TO_ENUM(head_r2.cmd), head_r2.nb_args);

//    int args_r2[head_r2.nb_args];
//    WAIT_FOR(args_r2, head_r2.nb_args, true); // todo: make sure 'true' is fine
//    PRINT_EXTRACTED("ACK", head_r2.nb_args, args_r2);


    // TP2 TODO:END

    for (unsigned int request_id = 0; request_id < num_request_per_client; request_id++)
    {

        // TP2 TODO
        // Vous devez ici coder, conjointement avec le corps de send request,
        // le protocole d'envoi de requête.

        send_request (ct->id, request_id, socket_fd);

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
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // todo: or INADDR_ANY ?
    if (connect(*socket_fd, &addr, sizeof(addr)) < 0) {
        perror("ct_code() | ERROR connecting ");
        exit(-1);
    } else {
        printf("--Client %d connected on socket_fd: %d\n", ct->id, *socket_fd);
    }
}
