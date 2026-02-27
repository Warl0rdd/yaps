#define POSIX_C_SOURCE 200112L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <netinet/in.h>

#include "utils/ipv4.h"

typedef struct {
    uint16_t min_port;
    uint16_t max_port;
    ipv4_t min_ip;
    ipv4_t max_ip;

    long timeout_ms; // milliseconds
} params_t;

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s -a <ip|ip-ip> -p <port|port-port> [-t <timeout_ms>]\n"
        "Examples:\n"
        "  %s -a 192.0.2.1 -p 22\n"
        "  %s -a 192.0.2.1-192.0.2.254 -p 1-1024 -t 500\n",
        prog, prog, prog);
}

static bool parse_port_range(const char *s, uint16_t *out_min, uint16_t *out_max) {
    if (!s || !out_min || !out_max) return false;

    char buf[64];
    if (strlen(s) >= sizeof(buf)) return false;
    strcpy(buf, s);

#ifdef _GNU_SOURCE
    char *tok0 = strtok_r(buf, "-", &saveptr);
#else
    char *tok0 = strtok(buf, "-");
#endif
    if (!tok0) return false;

    char *tok1 = NULL;
#ifdef _GNU_SOURCE
    tok1 = strtok_r(NULL, "-", &saveptr);
#else
    tok1 = strtok(NULL, "-");
#endif

    char *endptr = NULL;
    errno = 0;
    long v0 = strtol(tok0, &endptr, 10);
    if (errno || endptr == tok0 || v0 < 0 || v0 > 65535) return false;

    if (!tok1) {
        // single port
        *out_min = *out_max = (uint16_t)v0;
        return true;
    }

    errno = 0;
    long v1 = strtol(tok1, &endptr, 10);
    if (errno || endptr == tok1 || v1 < 0 || v1 > 65535) return false;

    if (v0 <= v1) {
        *out_min = (uint16_t)v0;
        *out_max = (uint16_t)v1;
    } else {
        // support reversed ranges like 1024-1 by swapping
        *out_min = (uint16_t)v1;
        *out_max = (uint16_t)v0;
    }
    return true;
}

static bool parse_ip_range(const char *s, ipv4_t *out_min, ipv4_t *out_max) {
    if (!s || !out_min || !out_max) return false;

    char buf[128];
    if (strlen(s) >= sizeof(buf)) return false;
    strcpy(buf, s);

#ifdef _GNU_SOURCE
    char *tok0 = strtok_r(buf, "-", &saveptr);
#else
    char *tok0 = strtok(buf, "-");
#endif
    if (!tok0) return false;

    char *tok1 = NULL;
#ifdef _GNU_SOURCE
    tok1 = strtok_r(NULL, "-", &saveptr);
#else
    tok1 = strtok(NULL, "-");
#endif

    ipv4_t ip0, ip1;
    if (!ipv4_from_str(tok0, &ip0)) return false;

    if (!tok1) {
        *out_min = *out_max = ip0;
        return true;
    }

    if (!ipv4_from_str(tok1, &ip1)) return false;

    if (ip0 <= ip1) {
        *out_min = ip0;
        *out_max = ip1;
    } else {
        // swap if reversed
        *out_min = ip1;
        *out_max = ip0;
    }
    return true;
}

int main(int argc, char **argv) {
    params_t params;
    // sensible defaults
    params.min_port = 0;
    params.max_port = 0;
    params.min_ip = 0;
    params.max_ip = 0;
    params.timeout_ms = 1000; // default 1s

    int opt;
    bool seen_a = false, seen_p = false;
    while ((opt = getopt(argc, argv, "a:p:t:h")) != -1) {
        switch (opt) {
            case 'a': {
                char tmp[128];
                if (strlen(optarg) >= sizeof(tmp)) {
                    fprintf(stderr, "Address argument too long\n");
                    return EXIT_FAILURE;
                }
                strcpy(tmp, optarg);
                if (!parse_ip_range(tmp, &params.min_ip, &params.max_ip)) {
                    fprintf(stderr, "Invalid IP or IP range: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                seen_a = true;
                break;
            }
            case 'p': {
                char tmp[64];
                if (strlen(optarg) >= sizeof(tmp)) {
                    fprintf(stderr, "Port argument too long\n");
                    return EXIT_FAILURE;
                }
                strcpy(tmp, optarg);
                if (!parse_port_range(tmp, &params.min_port, &params.max_port)) {
                    fprintf(stderr, "Invalid port or port range: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                seen_p = true;
                break;
            }
            case 't': {
                char *endptr = NULL;
                errno = 0;
                long v = strtol(optarg, &endptr, 10);
                if (errno || endptr == optarg || v < 0) {
                    fprintf(stderr, "Invalid timeout value: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                params.timeout_ms = v;
                break;
            }
            case 'h':
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (!seen_a || !seen_p) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // print parsed values
    char ipbuf_min[INET_ADDRSTRLEN];
    char ipbuf_max[INET_ADDRSTRLEN];
    if (!ipv4_to_str(params.min_ip, ipbuf_min, sizeof(ipbuf_min)) ||
        !ipv4_to_str(params.max_ip, ipbuf_max, sizeof(ipbuf_max))) {
        fprintf(stderr, "Error formatting IPs\n");
        return EXIT_FAILURE;
    }

    printf("IP range: %s - %s\n", ipbuf_min, ipbuf_max);
    printf("Port range: %u - %u\n", (unsigned)params.min_port, (unsigned)params.max_port);
    printf("Timeout: %ld ms\n", params.timeout_ms);

    // TODO

    return EXIT_SUCCESS;
}
