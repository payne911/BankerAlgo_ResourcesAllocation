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





            /*
             *#################################################
             *#              ADDITIONAL METHODS               #
             *#################################################
             */

bool read_all(int sockfd, void *buf, size_t obj_sz, int timeout) {
    /// Wrapper on "read_socket" to verify if the whole data was received.

    int ret = read_socket(sockfd, buf, obj_sz, timeout);
    if(ret != obj_sz) {
        return false;
    }
    return true;
}

bool send_header(int fd, cmd_header_t *data, size_t len) {
    /// Send the data through the socket.
    /// see: https://stackoverflow.com/a/49395422/9768291

    printf("_____send_header(): cmd_type(%d)=%s  nb_args=%d | fd:%d\n",
           data->cmd, TO_ENUM(data->cmd), data->nb_args, fd);

    while (len > 0) {
        ssize_t l = send(fd, data, len, MSG_NOSIGNAL);

        if (l > 0) {
            data += l;
            len  -= l;
        } else if (l == 0) {
            fprintf(stderr, "send_header(): empty?\n");
            break;
        } else if (errno == EINTR) { // a signal occured before any data was transmitted
            continue;
        } else {
            perror("send()");
            break;
        }
    }

    return (len == 0);
}


bool send_args(int fd, int *args, size_t len) {
    /// Send the data through the socket.
    /// see: https://stackoverflow.com/a/49395422/9768291

    printf("_____send_args(): length= %zu | fd:%d | arg0=%d\n", len, fd, args[0]);

    while (len > 0) {
        ssize_t l = send(fd, args, len, MSG_NOSIGNAL);

        if (l > 0) {
            args += l;
            len  -= l;
        } else if (l == 0) {
            fprintf(stderr, "send_args(): empty?\n");
            break;
        } else if (errno == EINTR) { // a signal occured before any data was transmitted
            continue;
        } else {
            perror("send()");
            break;
        }
    }

    return (len == 0);
}
