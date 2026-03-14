//
// Created by WarlOrd on 13.03.2026.
//

#ifndef YAPS_ICMP_H
#define YAPS_ICMP_H
#include <stdint.h>
#include <sys/types.h>

#include "utils/ipv4.h"

typedef enum {
    ICMP_ST_OK = 0,
    ICMP_ST_TIMEOUT,
    ICMP_ST_UNREACHABLE,
    ICMP_ST_ERROR,
    ICMP_ST_INTERNAL_ERROR = -1,
} icmp_status_t;

typedef struct __attribute__((packed)) { // fuck padding
    uint32_t magic;
    uint64_t timestamp; // monotonic nanoseconds
    uint32_t pid;
    uint32_t seq;
    uint64_t reserved; // for future use
    uint8_t filler[28];
} icmp_payload_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} icmp_header;

typedef struct {
    icmp_header header;
    icmp_payload_t payload;
} icmp_packet_t;

icmp_status_t icmp_ping_sweep(ipv4_t ip, u_long timeout);

#endif //YAPS_ICMP_H
