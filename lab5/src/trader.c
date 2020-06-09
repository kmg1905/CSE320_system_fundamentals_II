#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "trader.h"
#include "helper_functions.h"
#include "protocol.h"
#include "debug.h"

sem_t sem_traders;

typedef struct trader
{
    funds_t balance;
    quantity_t inventory;
    int fd;
    int ref_count;
    char *name;
    pthread_mutex_t mutex;
}TRADER;


TRADER *traders[MAX_TRADERS];
int rear;

int trader_init(){
    sem_init(&sem_traders,0,1);
    debug("Initialize trader module");
    for(int i=0;i<MAX_TRADERS;i++){
        traders[i] = NULL;
    }
    rear = 0;
    return 0;
}

void trader_fini(){
    debug("Finalize trader module");
    for(int i=0;i<MAX_TRADERS;i++){
        if(traders[i] != NULL){
            trader_unref(traders[i], "because exchange is shutting down");
            debug("%ld: Free trader %p [%s]",pthread_self(), traders[i], traders[i]->name);
            free(traders[i]->name);
            free(traders[i]);
            traders[i] = NULL;
        }
    }
    rear = 0;
}

TRADER *trader_login(int fd, char *name){
    sem_wait(&sem_traders);
    TRADER *trader = NULL;
    if(rear == MAX_TRADERS){
        free(name);
        sem_post(&sem_traders);
        return NULL;
    }
    int i=0;
    while(i<MAX_TRADERS && trader == NULL){
        if(traders[i] != NULL){
            if(strcmp(traders[i]->name,name)==0){
                trader = traders[i];
            }
        }
        i++;
    }

    if(trader == NULL){
        trader = malloc(sizeof(TRADER));
        traders[rear] = trader;
        rear++;
        debug("%ld: Create new trader %p [%s]", pthread_self(), trader, name);
        trader->fd = fd;
        trader->name = name;
        trader->ref_count = 0;
        trader->balance = 0;
        trader->inventory = 0;
        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&trader->mutex, &attr);
        trader_ref(trader,"for a newly created trader");
        trader_ref(trader,"because trader has logged in");
    }else if(trader != NULL && trader->fd == -1){
        debug("%ld: Login existing trader %p [%s]",pthread_self(), trader, name);
        trader->fd = fd;
        trader_ref(trader,"because trader has logged in");
        free(name);
    }else if(trader != NULL && trader->fd != -1){
        debug("%ld: Trader %s is already logged in",pthread_self(),name);
        trader = NULL;
        free(name);
    }
    sem_post(&sem_traders);

    return trader;
}

void trader_logout(TRADER *trader){
    pthread_mutex_lock(&trader->mutex);
    debug("%ld: Log out trader %p [%s]",pthread_self(), trader, trader->name);
    trader->fd = -1;
    trader_unref(trader, "because trader has logged out");
    pthread_mutex_unlock(&trader->mutex);
}

TRADER *trader_ref(TRADER *trader, char *why){
    pthread_mutex_lock(&trader->mutex);
    debug("%ld: Increasing the reference count of the trader %p [%s] (%d -> %d) %s",pthread_self(),trader,trader->name,trader->ref_count,trader->ref_count+1,why);
    trader->ref_count = trader->ref_count + 1;
    pthread_mutex_unlock(&trader->mutex);
    return trader;
}

void trader_unref(TRADER *trader, char *why){
    pthread_mutex_lock(&trader->mutex);
    debug("%ld: Decreasing the reference count of the trader %p [%s] (%d -> %d) %s",pthread_self(),trader,trader->name,trader->ref_count,trader->ref_count-1,why);
    trader->ref_count = trader->ref_count -1;
    pthread_mutex_unlock(&trader->mutex);
}

int trader_send_packet(TRADER *trader, BRS_PACKET_HEADER *pkt, void *data){
    pthread_mutex_lock(&trader->mutex);
    int output = 0;
    if(trader->fd != -1){
        debug("%ld: Send packet (clientfd=%d, type=%s) for trader %p [%s]",pthread_self(), trader->fd, packet_types[pkt->type], trader, trader->name);
        output = proto_send_packet(trader->fd, pkt, data);
    }else{
        debug("Error %s sending noitification",packet_types[pkt->type]);
    }
    pthread_mutex_unlock(&trader->mutex);
    return output;
}

/*
 * Broadcast a packet to all currently logged-in traders.
 *
 * @param pkt  The packet to be sent.
 * @param data  Data payload to be sent, or NULL if none.
 * @return 0 if broadcast succeeds, -1 otherwise.
 */
int trader_broadcast_packet(BRS_PACKET_HEADER *pkt, void *data){
    for(int i=0;i<MAX_TRADERS;i++){
        if(traders[i] != NULL && traders[i]->fd != -1){
            trader_send_packet(traders[i], pkt, data);
        }
    }
    return 0;
}

/*
 * Send an ACK packet to the client for a trader.
 *
 * @param trader  The TRADER object for the client who should receive
 * the packet.
 * @param infop  Pointer to the optional data payload for this packet,
 * or NULL if there is to be no payload.
 * @return 0 if transmission succeeds, -1 otherwise.
 */
int trader_send_ack(TRADER *trader, BRS_STATUS_INFO *info){
    pthread_mutex_lock(&trader->mutex);
    BRS_PACKET_HEADER hdr;
    int output = 0;
    memset(&hdr, 0, sizeof(BRS_PACKET_HEADER));
    hdr.type = BRS_ACK_PKT;
    if(info != NULL){
        hdr.size = htons(sizeof(BRS_STATUS_INFO));
        info->balance = htonl(trader->balance);
        info->inventory = htonl(trader->inventory);
    }else{
        hdr.size = htons(0);
    }
    output = trader_send_packet(trader,&hdr,info);
    pthread_mutex_unlock(&trader->mutex);
    return output;
}

/*
 * Send an NACK packet to the client for a trader.
 *
 * @param trader  The TRADER object for the client who should receive
 * the packet.
 * @return 0 if transmission succeeds, -1 otherwise.
 */
int trader_send_nack(TRADER *trader){
    pthread_mutex_lock(&trader->mutex);
    BRS_PACKET_HEADER hdr;
    int output;
    memset(&hdr, 0, sizeof(BRS_PACKET_HEADER));
    hdr.type = BRS_NACK_PKT;
    hdr.size = htonl(0);
    output = trader_send_packet(trader,&hdr,NULL);
    pthread_mutex_unlock(&trader->mutex);
    return output;
}

void trader_increase_balance(TRADER *trader, funds_t amount){
    sem_wait(&sem_traders);
    pthread_mutex_lock(&trader->mutex);
    debug("[%d] Increase balance (%u -> %u)",trader->fd,trader->balance,trader->balance + amount);
    trader->balance = trader->balance + amount;
    pthread_mutex_unlock(&trader->mutex);
    sem_post(&sem_traders);
}

int trader_decrease_balance(TRADER *trader, funds_t amount){
    sem_wait(&sem_traders);
    pthread_mutex_lock(&trader->mutex);
    int output;
    if(trader->balance >= amount){
        debug("[%d] Decrease balance (%u -> %u)",trader->fd,trader->balance,trader->balance - amount);
        trader->balance = trader->balance - amount;
        output = 0;
    }else{
        debug("[%d] Balance %u is less than debit amount %u",trader->fd,trader->balance,amount);
        output = -1;
    }
    pthread_mutex_unlock(&trader->mutex);
    sem_post(&sem_traders);
    return output;
}

void trader_increase_inventory(TRADER *trader, quantity_t quantity){
    sem_wait(&sem_traders);
    pthread_mutex_lock(&trader->mutex);
    debug("[%d] Increase inventory (%u -> %u)",trader->fd,trader->inventory,trader->inventory + quantity);
    trader->inventory = trader->inventory + quantity;
    pthread_mutex_unlock(&trader->mutex);
    sem_post(&sem_traders);
}

int trader_decrease_inventory(TRADER *trader, quantity_t quantity){
    sem_wait(&sem_traders);
   pthread_mutex_lock(&trader->mutex);
    int output;
    if(trader->inventory >= quantity){
        debug("[%d] Decrease inventory (%u -> %u)",trader->fd,trader->inventory,trader->inventory - quantity);
        trader->inventory = trader->inventory - quantity;
        output = 0;
    }else{
        debug("[%d] Inventory %u is less than quantity to decrease by %u",trader->fd,trader->inventory,quantity);
        output = -1;
    }
    pthread_mutex_unlock(&trader->mutex);
    sem_post(&sem_traders);
    return output;
}
