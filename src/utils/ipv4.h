#ifndef YAPS_IPV4_H
#define YAPS_IPV4_H

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint32_t ipv4_t;

bool ipv4_from_str(const char *s, ipv4_t *out);
char *ipv4_to_str(ipv4_t ip, char *buf, size_t buflen);

#endif //YAPS_IPV4_H
