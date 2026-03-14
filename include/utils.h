//
// Created by WarlOrd on 13.03.2026.
//

#ifndef YAPS_UTILS_H
#define YAPS_UTILS_H

#include <stdint.h>

int close_and_set_errno(int fd, int err);
uint64_t now_ns(void);

#endif //YAPS_UTILS_H
