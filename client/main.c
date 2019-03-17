#include "client_thread.h"

int
main (int argc, char *argv[argc + 1])
{
    if (argc < 5)
    {
        fprintf (stderr, "Usage: %s <port-nb> <nb-clients> <nb-requests> <resources>...\n",
                 argv[0]);
        exit (1);
    }

    port_number = atoi (argv[1]);
    int num_clients = atoi (argv[2]);
    num_request_per_client = atoi (argv[3]);
    num_resources = argc - 4;

    provisioned_resources = malloc (num_resources * sizeof (int));
    for (unsigned int i = 0; i < num_resources; i++)
        provisioned_resources[i] = atoi (argv[i + 4]);

    /* Added to initialize the server "gracefully". */
    bool success_on_init = ct_init_server();

    if(success_on_init) {
        client_thread *client_threads
                = malloc(num_clients * sizeof(client_thread));
        for (unsigned int i = 0; i < num_clients; i++)
            ct_init(&(client_threads[i]));

        for (unsigned int i = 0; i < num_clients; i++) {
            ct_create_and_start(&(client_threads[i]));
        }

        ct_wait_server();

        /* Free all the memory allocated. */
        free(client_threads);
    } else {

        /* Sends an `END` to the server because an error occured. */
        ct_wait_server();
    }

    // Affiche le journal.
    ct_print_results (stdout, true);
    FILE *fp = fopen("client.log", "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Could not print log");
        return EXIT_FAILURE;
    }
    ct_print_results (fp, false);
    fclose(fp);

    return EXIT_SUCCESS;
}
