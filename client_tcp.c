#include "client_tcp.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"

#define BUF_MAX 512

typedef struct connector_tcp_t {
    int fd;
    char buf[BUF_MAX];
} connector_tcp;

void connector_tcp_destructor(connector_tcp *self) {
    close(self->fd);
    free(self);
}

int connector_tcp_send(
        connector_tcp *self,
        const char *msg,
        size_t nbytes
) {
    if (write(self->fd, msg, nbytes) < 0) return -1;
    return 0;
}

const char *connector_tcp_recv(
        connector_tcp *self,
        size_t nbytes
) {
    memset(self->buf, 0, BUF_MAX);

    long bytes_read = 0;
    while (bytes_read < nbytes) {
        long _bytes_read = read(
                self->fd,
                &self->buf[bytes_read],
                nbytes - bytes_read
        );
        if (_bytes_read < 0) return NULL; // read definitely failed
        bytes_read += _bytes_read;
    }

    return self->buf;
}

int connector_tcp_buf_size() {
    return BUF_MAX;
}

int connect_to_tcp(struct sockaddr_storage addr) {
    int sock_fd = socket(addr.ss_family, SOCK_STREAM, 0);
    if (sock_fd == -1) return -1;

    if (connect(sock_fd, (struct sockaddr *) &addr, sizeof(addr))) return -1;

    return sock_fd;
}

int connector_from_tcp(
        connector *self,
        struct sockaddr_storage addr
) {
    connector_destructor(self);

    int sock_fd = connect_to_tcp(addr);
    if (sock_fd < 0) return 1;

    connector_tcp *p = malloc(sizeof(connector_tcp));
    memset(p, 0, sizeof(connector_tcp));
    p->fd = sock_fd;

    self->p = p;
    self->destructor = (void (*)(void *)) connector_tcp_destructor;
    self->send_func = (int (*)(void *, const char *, size_t)) connector_tcp_send;
    self->recv_func = (const char *(*)(void *, size_t)) connector_tcp_recv;
    self->buffer_size = &connector_tcp_buf_size;

    return 0;
}

