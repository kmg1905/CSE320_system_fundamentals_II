#include "csapp.h"
#include "client_registry.h"


typedef struct client_registry {
    int buf[FD_SETSIZE - 1];
    volatile int connect;
    volatile int front;
    int rear;

    sem_t mutex;
    sem_t connections;
    sem_t slots;
    sem_t items;

} CLIENT_REGISTRY;

CLIENT_REGISTRY *creg_init() {

    CLIENT_REGISTRY *buffer = Malloc(sizeof(CLIENT_REGISTRY));

    for (int i = 0; i < FD_SETSIZE - 1; i++)
        buffer->buf[i] = 0;

    buffer->front = buffer->rear = 0;
    buffer->connect = 0;
    Sem_init(&buffer->mutex, 0, 1);
    Sem_init(&buffer->slots, 0, FD_SETSIZE - 1);
    Sem_init(&buffer->connections, 0, 0);
    Sem_init(&buffer->items, 0, 0);

    return buffer;
}

void creg_fini(CLIENT_REGISTRY *cr) {

    Free(cr);
}

int creg_register(CLIENT_REGISTRY *cr, int fd) {

    P(&cr->slots);
    P(&cr->mutex);
    for (int i = 0; i < FD_SETSIZE - 1; i++) {
        if (cr->buf[i] == 0) {
            cr->buf[i] = fd;
            break;
        }
    }

    cr->connect = cr->connect + 1;
    V(&cr->mutex);
    V(&cr->items);

    return 0;
}

int creg_unregister(CLIENT_REGISTRY *cr, int fd) {

    P(&cr->items);
    P(&cr->mutex);
    for (int i = 0; i < FD_SETSIZE - 1; i ++) {
        if (cr->buf[i] == fd) {
            cr->buf[i] = 0;
            break;
        }
    }
    cr->connect = cr->connect - 1;
    if (cr->connect == 0) {
        V(&cr->connections);
    }
    V(&cr->mutex);
    V(&cr->slots);

    return 0;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr) {

    P(&cr->connections);
    return;
}

void creg_shutdown_all(CLIENT_REGISTRY *cr) {

    for (int i = 0; i < FD_SETSIZE - 1; i ++) {
            if (cr->buf[i] != 0) {
                shutdown(cr->buf[i], SHUT_RD);
                cr->buf[i] = 0;
            }
        }
    return;
}