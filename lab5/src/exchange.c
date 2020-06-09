#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "exchange.h"
#include "debug.h"
#include "exchange_functions.h"


void removeFromLinkedList(ORDER **head, ORDER *node){
    if(node->prev == NULL){
        *head = node->next;
    }else{
        node->prev->next = node->next;
    }

    if(node->next != NULL){
        node->next->prev = node->prev;
    }
}

void *matchmaker(void *arg){
    EXCHANGE *exchange = (EXCHANGE *) arg;
    debug("Matchmaker %ld for exchange %p starting", pthread_self(),exchange);
    while(1){
        debug("Matchmaker %ld for exchange %p sleeping", pthread_self(),exchange);
        sem_wait(&exchange->matcher_mutex);
        sem_wait(&exchange->mutex);
        debug("Matchmaker %ld for exchange %p active",pthread_self(),exchange);
        if(exchange->check == 1){
            debug("Matchmaker %ld for exchange %p terminating",pthread_self(),exchange);
            sem_post(&exchange->matcher_wait);
            pthread_cancel(pthread_self());
            return NULL;
        }
        match_order(exchange);
        debug("Matchmaker %ld sees no possible trades",pthread_self());
        sem_post(&exchange->mutex);
    }
    return NULL;
}

EXCHANGE *exchange_init(){
    EXCHANGE *exchange = malloc(sizeof(EXCHANGE));
    exchange->buy_orders = NULL;
    exchange->sell_orders = NULL;
    exchange->buy_orders_tail = NULL;
    exchange->sell_orders_tail = NULL;
    exchange->last_price = 0;
    exchange->orderid = 1;
    exchange->bid = 0;
    exchange->check = 0;
    exchange->ask = UINT32_MAX;
    sem_init(&exchange->mutex,0,1);
    sem_init(&exchange->matcher_mutex,0,0);
    sem_init(&exchange->matcher_wait,0,0);
    debug("Initialized exchange %p",exchange);
    pthread_create(&exchange->tid, NULL, matchmaker, exchange);
    return exchange;
}

void exchange_fini(EXCHANGE *xchg){
    sem_wait(&xchg->mutex);
    debug("Finalizing exchange %p",xchg);
    ORDER *buy_orders = xchg->buy_orders;
    ORDER *sell_orders = xchg->sell_orders;
    ORDER *order;
    while(buy_orders != NULL){
        order = buy_orders;
        buy_orders = order->next;
        free(order);
    }

    while(sell_orders != NULL){
        order = sell_orders;
        sell_orders = order->next;
        free(order);
    }

    xchg->check = 1;

    sem_post(&xchg->mutex);
    sem_post(&xchg->matcher_mutex);
    sem_wait(&xchg->matcher_wait);
    pthread_join(xchg->tid,NULL);
    free(xchg);
}

/*
 * Get the current status of the exchange.
 */
void exchange_get_status(EXCHANGE *xchg, BRS_STATUS_INFO *infop){
    debug("Get status of exchange %p",xchg);
    infop->bid = htonl(xchg->bid);
    if(xchg->ask == UINT32_MAX){
        infop->ask = htonl(0);
    }else{
        infop->ask = htonl(xchg->ask);
    }
    infop->last = htonl(xchg->last_price);
}

/*
 * Post a buy order on the exchange on behalf of a trader.
 * The trader is stored with the order, and its reference count is
 * increased by one to account for the stored pointer.
 * Funds equal to the maximum possible cost of the order are
 * encumbered by removing them from the trader's account.
 * A POSTED packet containing details of the order is broadcast
 * to all logged-in traders.
 *
 * @param xchg  The exchange to which the order is to be posted.
 * @param trader  The trader on whose behalf the order is to be posted.
 * @param quantity  The quantity to be bought.
 * @param price  The maximum price to be paid per unit.
 * @return  The order ID assigned to the new order, if successfully posted,
 * otherwise 0.
 */
orderid_t exchange_post_buy(EXCHANGE *xchg, TRADER *trader, quantity_t quantity,
                funds_t price){
    sem_wait(&xchg->mutex);
    orderid_t output = 0;
    if(trader_decrease_balance(trader,quantity * price) == 0){
        ORDER *order = malloc(sizeof(ORDER));
        BRS_PACKET_HEADER hdr;
        BRS_NOTIFY_INFO info;

        memset(&hdr, 0, sizeof(BRS_PACKET_HEADER));
        //Initialize buy order
        order->id = xchg->orderid;
        order->type = BUY_ORDER;
        order->quant = quantity;
        order->price = price;
        order->prev = NULL;
        order->next = NULL;

        //Insert in buy orders
        list_insert(&xchg->buy_orders, &xchg->buy_orders_tail, order);
        order->trader_ref = trader;
        sem_init(&order->mutex,0,1);

        //Increasing reference of the trader
        trader_ref(trader,"to place in new order");

        //Updating exchange
        if(xchg->bid < price){
            xchg->bid = price;
        }
        // xchg->buy_orders = order;
        xchg->orderid = xchg->orderid + 1;
        debug("Exchange %p posteding buy order %d for trader %p, quantity %u, max price %u",xchg,order->id,trader, quantity, price);
        sem_post(&xchg->matcher_mutex);
        printExchange(xchg);

        hdr.type = BRS_POSTED_PKT;
        hdr.size = htons(sizeof(BRS_NOTIFY_INFO));
        info.buyer = htonl(order->id);
        info.seller = htonl(0);
        info.quantity = htonl(quantity);
        info.price = htonl(price);

        trader_broadcast_packet(&hdr, &info);
        output = order->id;
    }
    sem_post(&xchg->mutex);
    return output;
}

/*
 * Post a sell order on the exchange on behalf of a trader.
 * The trader is stored with the order, and its reference count is
 * increased by one to account for the stored pointer.
 * Inventory equal to the amount of the order is
 * encumbered by removing it from the trader's account.
 * A POSTED packet containing details of the order is broadcast
 * to all logged-in traders.
 *
 * @param xchg  The exchange to which the order is to be posted.
 * @param trader  The trader on whose behalf the order is to be posted.
 * @param quantity  The quantity to be sold.
 * @param price  The minimum sale price per unit.
 * @return  The order ID assigned to the new order, if successfully posted,
 * otherwise 0.
 */
orderid_t exchange_post_sell(EXCHANGE *xchg, TRADER *trader, quantity_t quantity,
                 funds_t price){
    sem_wait(&xchg->mutex);
    orderid_t output = 0;
    if(trader_decrease_inventory(trader, quantity) == 0){
        ORDER *order = malloc(sizeof(ORDER));
        BRS_PACKET_HEADER hdr;
        BRS_NOTIFY_INFO info;
        memset(&hdr, 0, sizeof(BRS_PACKET_HEADER));
        //Inserting order in buy orders
        order->id = xchg->orderid;
        order->type = SELL_ORDER;
        order->quant = quantity;
        order->price = price;
        order->prev = NULL;
        order->next = NULL;

        //Insert in sell orders
        list_insert(&xchg->sell_orders, &xchg->sell_orders_tail, order);
        order->trader_ref = trader;
        sem_init(&order->mutex,0,1);

        //Increasing reference of the trader
        trader_ref(trader,"to place in new order");

        //Updating exchange
        if(xchg->ask > price){
            xchg->ask = price;
        }

        xchg->orderid = xchg->orderid + 1;

        debug("Exchange %p posting buy order %d for trader %p, quantity %u, max price %u",xchg,order->id,trader, quantity, price);
        sem_post(&xchg->matcher_mutex);

        //Printing the exchange
        printExchange(xchg);

        hdr.type = BRS_POSTED_PKT;
        hdr.size = htons(sizeof(BRS_NOTIFY_INFO));
        info.seller = htonl(order->id);
        info.buyer = htonl(0);
        info.quantity = htonl(quantity);
        info.price = htonl(price);

        trader_broadcast_packet(&hdr, &info);
        output = order->id;
    }
    sem_post(&xchg->mutex);
    return output;
}

/*
 * Attempt to cancel a pending order.
 * If successful, the quantity of the canceled order is returned in a variable,
 * and a CANCELED packet containing details of the canceled order is
 * broadcast to all logged-in traders.
 *
 * @param xchg  The exchange from which the order is to be cancelled.
 * @param trader  The trader cancelling the order is to be posted,
 * which must be the same as the trader who originally posted the order.
 * @param id  The order ID of the order to be cancelled.
 * @param quantity  Pointer to a variable in which to return the quantity
 * of the order that was canceled.  Note that this need not be the same as
 * the original order amount, as the order could have been partially
 * fulfilled by trades.
 * @return  0 if the order was successfully cancelled, -1 otherwise.
 * Note that cancellation might fail if a trade fulfills and removes the
 * order before this function attempts to cancel it.
 */
int exchange_cancel(EXCHANGE *xchg, TRADER *trader, orderid_t order,
            quantity_t *quantity){
    sem_wait(&xchg->mutex);
    ORDER *curr_order = xchg->buy_orders;
    ORDER *req_order = NULL;
    BRS_PACKET_HEADER hdr;
    BRS_NOTIFY_INFO info;
    while(curr_order != NULL && req_order == NULL){
        if(curr_order->id == order && curr_order->trader_ref == trader){
            req_order = curr_order;
        }
        curr_order = curr_order->next;
    }

    curr_order = xchg->sell_orders;
    while(curr_order != NULL && req_order == NULL){
        if(curr_order->id == order && curr_order->trader_ref == trader){
            req_order = curr_order;
        }
        curr_order = curr_order->next;
    }

    if(req_order == NULL){
        sem_post(&xchg->mutex);
        return -1;
    }


    if(req_order->type == BUY_ORDER){
        list_remove(&xchg->buy_orders, &xchg->buy_orders_tail,req_order);
    }else if(req_order->type == SELL_ORDER){
        list_remove(&xchg->sell_orders, &xchg->sell_orders_tail,req_order);
    }

    if(xchg->buy_orders_tail != NULL){
        xchg->bid = xchg->buy_orders_tail->price;
    }else{
        xchg->bid = 0;
    }

    if(xchg->sell_orders != NULL){
        xchg->ask = xchg->sell_orders->price;
    }else{
        xchg->ask = UINT32_MAX;
    }

    printExchange(xchg);

    if(req_order->type == BUY_ORDER){
        info.seller = htonl(0);
        info.buyer = htonl(req_order->id);
        trader_increase_balance(trader, req_order->quant * req_order->price);
        trader_unref(trader,"in order being freed");
    }else{
        info.seller = htonl(req_order->id);
        info.buyer = htonl(0);
        trader_increase_inventory(trader, req_order->quant);
        trader_unref(trader,"in order being freed");
    }

    hdr.type = BRS_CANCELED_PKT;
    hdr.size = htons(sizeof(BRS_NOTIFY_INFO));
    info.quantity = htonl(req_order->quant);
    *quantity = req_order->quant;
    info.price = htonl(0);
    trader_broadcast_packet(&hdr, &info);
    free(req_order);
    sem_post(&xchg->mutex);
    return 0;
}



