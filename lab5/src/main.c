#include <stdlib.h>
#include <sys/signal.h>
#include <stdlib.h>
#include <string.h>

#include "client_registry.h"
#include "exchange.h"
#include "trader.h"
#include "debug.h"
#include "csapp.h"
#include "server.h"

extern EXCHANGE *exchange;
extern CLIENT_REGISTRY *client_registry;

static void terminate(int status);

void sighup_handler(int sighup) {

    if (sighup == SIGHUP)
        terminate(0);

}

/*
 * "Bourse" exchange server.
 *
 * Usage: bourse <port>
 */
int main(int argc, char* argv[]){

    if (argc != 3) {
        printf("bin/bourse -p <port>");
        exit(-1);
    }

    if (!strcmp(argv[2], "-p" )) {
        printf("incorrect argument %s\n", argv[2]);
        exit(-1);
    }

    int listenfd;
    int *connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;



    struct sigaction options;
    memset(&options, 0, sizeof(options));
    options.sa_handler = sighup_handler;

    if (sigaction(SIGHUP, &options, NULL) < 0) {
        perror("sigaction");
        exit(-1);
    }

    // Perform required initializations of the client_registry,
    // maze, and player modules.
    client_registry = creg_init();
    exchange = exchange_init();
    if (trader_init() < 0){
        exit(-1);
    }

    listenfd = Open_listenfd(argv[2]);

    while(1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA*) &clientaddr, &clientlen);
        Pthread_create(&tid, NULL, brs_client_service, connfd);
    }


}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    exchange_fini(exchange);
    trader_fini();

    debug("Bourse server terminating");
    exit(status);
}
