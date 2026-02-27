#include "ipv4.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  // Ensure linking ws2_32.lib in build
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
#endif

bool ipv4_from_str(const char *s, ipv4_t *out) {
    if (!s || !out) return false;
    struct in_addr a;
    // inet_pton returns 1 on success
    int r = inet_pton(AF_INET, s, &a);
    if (r == 1) {
        // a.s_addr is in network byte order -> convert to host order
        *out = ntohl(a.s_addr);
        return true;
    }
    return false;
}

char *ipv4_to_str(ipv4_t ip, char *buf, size_t buflen) {
    if (!buf || buflen < INET_ADDRSTRLEN) return NULL;
    struct in_addr a;
    // convert host-order stored ip to network order for inet_ntop
    a.s_addr = htonl(ip);
    if (inet_ntop(AF_INET, &a, buf, (socklen_t)buflen) == NULL) {
        return NULL;
    }
    return buf;
}
