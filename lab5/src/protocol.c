#include <time.h>
#include "protocol.h"
#include "csapp.h"


int proto_send_packet(int fd, BRS_PACKET_HEADER *hdr, void *payload) {

    int size = ntohs(hdr->size);
    struct timespec timestamp;
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    //converting to network format
    hdr->timestamp_sec = htonl(timestamp.tv_sec);
    hdr->timestamp_nsec = htonl(timestamp.tv_nsec);

    if (rio_writen(fd, hdr, sizeof(BRS_PACKET_HEADER)) < 0) {

        return -1;
    }

    if (size > 0) {
        if (rio_writen(fd, payload, (size)) < 0) {
            return -1;
        }
    }

    return 0;
}


int proto_recv_packet(int fd, BRS_PACKET_HEADER *hdr, void **payloadp) {

    if (rio_readn(fd, hdr, sizeof(BRS_PACKET_HEADER)) <= 0) {
        return -1;
    }

    hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
    hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);
    uint16_t size = ntohs(hdr->size);
    if (size > 0) {
        *payloadp = Malloc(size);
        if (rio_readn(fd, *payloadp, size) < 0) {
            return -1;
        }
    }
    return 0;
}

