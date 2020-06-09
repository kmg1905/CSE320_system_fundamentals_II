#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "exchange_functions.h"
#include "debug.h"

void list_insert(ORDER **head, ORDER **tail, ORDER *node){
    if(*head == NULL && *tail == NULL){
        *head = node;
        *tail = node;
        return;
    }

    ORDER *curr = *head;
    ORDER *temp = NULL;

    while(curr != NULL && curr->price <= node->price){
        curr = curr->next;
    }

    if(curr == NULL){
        (*tail)->next = node;
        node->next = NULL;
        node->prev = *tail;
        *tail = node;
    }else{
        if(curr->prev == NULL){
            *head = node;
        }else{
            curr->prev->next = node;
        }
        temp = curr->prev;
        curr->prev = node;
        node->next = curr;
        node->prev = temp;
    }
}

void list_remove(ORDER **head, ORDER **tail, ORDER *node){
    if(node->next == NULL){
        *tail = node->prev;
    }else{
        node->next->prev = node->prev;
    }

    if(node->prev == NULL){
        *head = node->next;
    }else{
        node->prev->next = node->next;
    }
}

void send_traded_packets(BRS_PACKET_HEADER *hdr, BRS_NOTIFY_INFO *info,uint8_t type, funds_t quantity, funds_t price, int seller_id, int buyer_id){
    hdr->type = type;
    hdr->size = htons(sizeof(BRS_NOTIFY_INFO));
    info->seller = htonl(seller_id);
    info->buyer = htonl(buyer_id);
    info->quantity = htonl(quantity);
    info->price = htonl(price);
}

void printExchange(EXCHANGE *xchg){
    printf("Last trade price: %u\n", xchg->last_price);
    printf("%s\n","Buy orders:");
    ORDER *order = xchg->buy_orders;
    while(order != NULL){
        printf("[id: %d, trader: %p, type: %d, quant: %u, price: %u]  ",order->id, order->trader_ref, order->type, order->quant, order->price);
        order = order->next;
    }

    printf("\n%s\n","Sell orders:");
    order = xchg->sell_orders;
    if(order == NULL){
        printf("\n");
    }else{
        while(order != NULL){
            printf("[id: %d, trader: %p, type: %d, quant: %u, price: %u]  ",order->id, order->trader_ref, order->type, order->quant, order->price);
            order = order->next;
        }
        printf("\n");
    }
}

void match_order(EXCHANGE *exchange){
    ORDER *sell_order = exchange->sell_orders;
    ORDER *buy_order = exchange->buy_orders_tail;
    ORDER *buy_temp;
    ORDER *sell_temp;
    funds_t price;
    quantity_t quantity;
    BRS_PACKET_HEADER hdr;
    BRS_NOTIFY_INFO info;
    memset(&hdr, 0 , sizeof(BRS_PACKET_HEADER));
    memset(&info, 0 , sizeof(BRS_NOTIFY_INFO));
    while(sell_order != NULL){
        sell_temp = sell_order->next;
        buy_order = exchange->buy_orders_tail;
        while(buy_order != NULL && sell_order->price <= buy_order->price){
            price = (sell_order->price > exchange->last_price) ? sell_order->price : exchange->last_price;
            price = (buy_order->price < price) ? buy_order->price : price;
            quantity = sell_order->quant < buy_order->quant ? sell_order->quant : buy_order->quant;

            debug("Matchmaker %ld executing trade for quantity %u at price %u (sell order %d, buy order %d)",
                pthread_self(),quantity,price,sell_order->id,buy_order->id);

            trader_increase_balance(sell_order->trader_ref, quantity * price);
            trader_increase_inventory(buy_order->trader_ref, quantity);
            if(price < (buy_order->price)){
                trader_increase_balance(buy_order->trader_ref, quantity * (buy_order->price - price));
            }

            //Sending bought packet
            send_traded_packets(&hdr, &info, BRS_BOUGHT_PKT, quantity, price, sell_order->id, buy_order->id);
            trader_send_packet(buy_order->trader_ref,&hdr, &info);

            //Sending sold packet
            send_traded_packets(&hdr, &info, BRS_SOLD_PKT, quantity, price, sell_order->id, buy_order->id);
            trader_send_packet(sell_order->trader_ref,&hdr, &info);

            //Broadcasting traded packet
            send_traded_packets(&hdr, &info, BRS_TRADED_PKT, quantity, price, sell_order->id, buy_order->id);
            trader_broadcast_packet(&hdr, &info);

            sell_order->quant -= quantity;
            buy_order->quant -= quantity;
            exchange->last_price = price;

            buy_temp = buy_order->prev;
            if(buy_order->quant == 0){
                list_remove(&exchange->buy_orders,&exchange->buy_orders_tail,buy_order);
                trader_unref(buy_order->trader_ref,"in order being freed");
                free(buy_order);
            }

            if(exchange->buy_orders_tail != NULL){
                exchange->bid = exchange->buy_orders_tail->price;
            }else{
                exchange->bid = 0;
            }

            buy_order = buy_temp;


            if(sell_order->quant == 0){
                list_remove(&exchange->sell_orders,&exchange->sell_orders_tail,sell_order);
                trader_unref(sell_order->trader_ref,"in order being freed");
                if(exchange->sell_orders != NULL){
                    exchange->ask = exchange->sell_orders->price;
                }else{
                    exchange->ask = UINT32_MAX;
                }
                free(sell_order);
                printExchange(exchange);
                break;
            }

            if(exchange->sell_orders != NULL){
                exchange->ask = exchange->sell_orders->price;
            }else{
                exchange->ask = UINT32_MAX;
            }

            printExchange(exchange);
        }
        sell_order = sell_temp;
    }
    return;
}