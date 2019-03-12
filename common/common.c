#include "common.h"

ssize_t read_socket(int sockfd, void *buf, size_t obj_sz, int timeout) {
    int ret;
    int len = 0;

    struct pollfd fds[1];
    fds->fd = sockfd;
    fds->events = POLLIN;
    fds->revents = 0;

    do {
        // wait for data or timeout
        ret = poll(fds, 1, timeout);

        if (ret > 0) {
            if (fds->revents & POLLIN) {
                ret = recv(sockfd, (char*)buf + len, obj_sz - len, 0);
                //printf("read_socket() : %d %d %p %zu\n", ret, len, buf, sizeof(buf));
                if (ret < 0) {
                    // abort connection
                    perror("recv()");
                    return -1;
                }
                len += ret;
            }
        } else {
            // TCP error or timeout
            if (ret < 0) {
                perror("poll()");
            }
            break;
        }
    } while (ret != 0 && len < obj_sz);
    return ret;
}
