//
// Created by WarlOrd on 13.03.2026.
//

#include "utils.h"

#include <unistd.h>
#include <_time.h>
#include <sys/errno.h>

int close_and_set_errno(int fd, int err) {
    int saved = errno;
    close(fd);
    errno = err ? err : saved;
    return -1;
}

uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
