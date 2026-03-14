//
// Created by WarlOrd on 13.03.2026.
//

#include "icmp.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "utils.h"
#include "utils/ipv4.h"

uint16_t icmp_checksum(const void *buf, size_t len) {
    uint32_t sum = 0;
    const uint16_t *p = buf;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len) sum += *(const uint8_t*)p;           // trailing byte
    while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
    return (uint16_t)~sum;
}

static icmp_packet_t construct_icmp_packet(void) {
    icmp_packet_t packet;
    memset(&packet, 0, sizeof(packet));

    // construct a header
    packet.header.type = 8;
    packet.header.code = 0;
    packet.header.checksum = 0;
    packet.header.seq = 0;
    packet.header.id = htons((uint16_t)getpid()); // idk how this one works tbh but it looks cool

    // payload
    packet.payload.magic = 0x59415053ULL; // "YAPS"
    packet.payload.seq = 0;
    packet.payload.timestamp = now_ns();
    packet.payload.pid = (uint32_t)getpid();
    memset(packet.payload.filler, 0xAA, sizeof(packet.payload.filler));

    packet.header.checksum = icmp_checksum(&packet, sizeof(packet));

    return packet;
}

icmp_status_t icmp_ping_sweep(ipv4_t ip, u_long timeout) { // timestamp is not being used rn but I hope I'll implement this later
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        return ICMP_ST_INTERNAL_ERROR;
    }

    struct sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = htonl(ip);

    icmp_packet_t packet = construct_icmp_packet();

    if (sendto(sock, &packet, sizeof(packet), 0, (struct sockaddr*)&to, sizeof(to)) < 0) {
        close(sock);
        return ICMP_ST_INTERNAL_ERROR;
    }

    struct pollfd pfd = {
        .fd = sock,
        .events = POLLIN
    };

    int ret = poll(&pfd, 1, timeout);

    if (ret == 0) {
        close(sock);
        return ICMP_ST_TIMEOUT;
    } else if (ret < 0) {
        close_and_set_errno(sock, errno);
        return ICMP_ST_INTERNAL_ERROR;
    } else {
        uint8_t buf[1500];

        ssize_t n = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
        if (n < 0) {
            close(sock);
            return ICMP_ST_INTERNAL_ERROR;
        }

        uint8_t ihl = (buf[0] & 0x0F) * 4;

        icmp_header *icmp = (icmp_header *)(buf + ihl);

        if (icmp->type == 0 && icmp->id == packet.header.id) {
            close(sock);
            return ICMP_ST_OK;
        }
        if (icmp->type == 3) {
            close(sock);
            return ICMP_ST_UNREACHABLE;
        }
        close(sock);
        return ICMP_ST_ERROR;
    }
}
