#include "csapp.h"
#include "client_registry.h"
#include "trader.h"
#include "protocol.h"
#include "exchange.h"
#include "debug.h"
#include "server.h"

extern CLIENT_REGISTRY *client_registry;
extern EXCHANGE *exchange;

void *brs_client_service(void *arg) {

    int connfd = *((int*)arg);
    Pthread_detach(pthread_self());
    Free(arg);

    creg_register(client_registry, connfd);
    TRADER *trader1;

    int loggedin = 0;
    void *payload;
    BRS_PACKET_HEADER hdr;
    BRS_STATUS_INFO info;
    int orderid;
    while (1) {

        if ((proto_recv_packet(connfd, &hdr, &payload)) <= 0) {
            creg_unregister(client_registry, connfd);
            return NULL;
        }

        if (hdr.type == BRS_LOGIN_PKT) {
            trader1 = trader_login(connfd, (char*)payload);

            if (trader1 == NULL) {
                BRS_PACKET_HEADER hdr2;
                hdr2.type = BRS_NACK_PKT;
                hdr2.size = 0;
                proto_send_packet(connfd, &hdr2, NULL);
                loggedin = 0;
            }
            else {
                if (trader_send_ack(trader1, NULL) < 0) {
                    return NULL;
                }
                loggedin = 1;
            }


        }


        if (loggedin == 1) {

            if (hdr.type == BRS_STATUS_PKT) {
                exchange_get_status(exchange, &info);
                trader_send_ack(trader1, &info);
                printf("Status Packet sent to fd %d \n", connfd);
            }

            else if (hdr.type == BRS_DEPOSIT_PKT) {
                    BRS_FUNDS_INFO *deposit = (BRS_FUNDS_INFO*)payload;
                    trader_increase_balance(trader1, ntohl(deposit->amount));
                    exchange_get_status(exchange, &info);
                    trader_send_ack(trader1, &info);
            }
            else if (hdr.type == BRS_WITHDRAW_PKT) {
                        BRS_FUNDS_INFO *deposit = (BRS_FUNDS_INFO*)payload;
                    if (trader_decrease_balance(trader1, ntohl(deposit->amount)) == 0) {
                            exchange_get_status(exchange, &info);
                            trader_send_ack(trader1, &info);
                        }
                    else {
                        trader_send_nack(trader1);
                    }
            }
            else if (hdr.type == BRS_ESCROW_PKT) {
                BRS_ESCROW_INFO *deposit = (BRS_ESCROW_INFO*)payload;
                trader_increase_inventory(trader1, ntohl(deposit->quantity));
                exchange_get_status(exchange, &info);
                trader_send_ack(trader1, &info);

            }
            else if (hdr.type == BRS_RELEASE_PKT) {
                BRS_ESCROW_INFO *release = (BRS_ESCROW_INFO*)payload;
                if (trader_decrease_inventory(trader1, ntohl(release->quantity)) == 0) {
                        exchange_get_status(exchange, &info);
                        trader_send_ack(trader1, &info);
                    }
                else {
                    trader_send_nack(trader1);
                }

            }
            else if (hdr.type == BRS_BUY_PKT) {
                BRS_ORDER_INFO *buy = (BRS_ORDER_INFO*)payload;

                orderid = exchange_post_buy(exchange, trader1,
                    htonl(buy->quantity), ntohl(buy->price));
                    if (orderid != 0) {
                        exchange_get_status(exchange, &info);
                        info.orderid = orderid;
                        trader_send_ack(trader1, &info);
                    }
                    else {
                        trader_send_nack(trader1);
                    }

            }
            else if (hdr.type == BRS_SELL_PKT) {
                BRS_ORDER_INFO *sell = (BRS_ORDER_INFO*)payload;
                orderid = exchange_post_sell(exchange, trader1,
                    ntohl(sell->quantity), ntohl(sell->price));
                if (orderid != 0 ) {
                        exchange_get_status(exchange, &info);
                        info.orderid = orderid;
                        trader_send_ack(trader1, &info);
                }
                else {
                    trader_send_nack(trader1);
                }

            }
            else if (hdr.type == BRS_CANCEL_PKT) {
                BRS_CANCEL_INFO *cancel = (BRS_CANCEL_INFO*)payload;
                quantity_t quantity;
                if (exchange_cancel(exchange, trader1,
                    ntohl(cancel->order), &quantity)!= 0) {
                        exchange_get_status(exchange, &info);
                        info.quantity = quantity;
                        info.orderid = cancel->order;
                        trader_send_ack(trader1, &info);

                }
                else {
                    trader_send_nack(trader1);
                }

            }
        else {
            debug("Login required");
            }
        }

    }


    return NULL;
}