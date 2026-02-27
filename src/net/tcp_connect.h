#ifndef YAPS_TCP_CONNECT_H
#define YAPS_TCP_CONNECT_H
#include <stdint.h>
#include <sys/types.h>

#include "utils/ipv4.h"

typedef enum {
    TCP_ST_OK = 0,        // connected
    TCP_ST_CLOSED,        // connection refused / RST
    TCP_ST_TIMEOUT,       // timed out
    TCP_ST_ERROR = -1,         // internal error (see err_no)
} tcp_status_t;

tcp_status_t tcp_connect(ipv4_t ip, uint16_t port, u_long timeout);

#endif //YAPS_TCP_CONNECT_H
