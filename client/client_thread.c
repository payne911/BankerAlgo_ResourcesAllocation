/* This `define` tells unistd to define usleep and random.  */
#define _XOPEN_SOURCE 500

#include "client_thread.h"
#include <netinet/in.h> // for `struct sockaddr_in`
#include <errno.h>

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

    cmd_header_t header;
    frame init;
    int test[] = {1,3,2,5,6,2425};
    setup_frame(&init, &header, REQ, 6, test); // todo: implement the proper request
    send_data(&socket_fd, &init, sizeof(init));



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

    /* Ask for connection. */
    cmd_header_t header;
    frame init;
    setup_frame(&init, &header, mBEGIN);
    send_data(&socket_fd, &init, sizeof(init));

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


bool send_data(int *fd, frame *data, size_t len) { //todo: rewrite
    /// Send the data through the socket.
    /// see: https://stackoverflow.com/a/49395422/9768291

    while (len > 0) {
        printf("send_data(): %d %d %d | fd:%d\n", data->header.cmd, data->header.nb_args, data->args[0], *fd);
        ssize_t l = send(*fd, data, len, MSG_NOSIGNAL);

        if (l > 0) {
            data += l;
            len  -= l;
        } else if (l == 0) {
            fprintf(stderr, "empty reply\n");
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

void setup_frame(struct frame *frame, struct cmd_header_t *header,
                 int cmd_type, int nb_args, int args[]) {
    /// To initialize the `struct` to be sent through the socket.

    // header
    header->cmd        = cmd_type;
    header->nb_args    = nb_args;
    frame->header      = *header;

    // cmd
    frame->args        = malloc(nb_args * sizeof(int)); // todo: OOM
    for(int i=0 ; i<nb_args; i++) {
        frame->args[i] = args[i];
    }
}
