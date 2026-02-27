// Basic TCP connection scan logic
// also I fucking HATE windows socket programming so this file's logic will NOT be available on windows
// thus it probably won't compile but idk I'll deal with it later

#include "tcp_connect.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>

#include "utils/ipv4.h"

static int close_and_set_errno(int fd, int err) {
    int saved = errno;
    close(fd);
    errno = err ? err : saved;
    return -1;
}

tcp_status_t tcp_connect(ipv4_t ip, uint16_t port, u_long timeout) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
       return TCP_ST_ERROR;
    }

    int flags = 0;

    // set non-blocking
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        return close_and_set_errno(sock, errno);
    }

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ip);

    int ret = connect(sock, (struct sockaddr *) &addr, sizeof(addr));
    if (ret == 0) { // connected immediately
        if (fcntl(sock, F_SETFL, flags) == -1) {
            return close_and_set_errno(sock, errno);
        }
        return TCP_ST_OK;
    }

    if (ret < 0 && errno != EINPROGRESS && errno != EWOULDBLOCK) {
        return close_and_set_errno(sock, errno);
    }

    struct pollfd fds;
    fds.fd = sock;
    fds.events = POLLOUT;
    fds.revents = 0;

    int pret;
    do {
        pret = poll(&fds, 1, timeout);
    } while (pret < 0 && errno == EINTR);

    if (pret == 0) {
        // timeout
        close_and_set_errno(sock, ETIMEDOUT);
        return TCP_ST_TIMEOUT;
    } else if (pret < 0) {
        return close_and_set_errno(sock, errno);
    }

    int err = 0;
    socklen_t len = sizeof(err);

    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
        close_and_set_errno(sock, errno);
        return TCP_ST_ERROR;
    }
    if (err != 0) {
        close_and_set_errno(sock, err);
        return TCP_ST_CLOSED;
    }

    close(sock);
    return TCP_ST_OK;
}
