#include "server_tcp.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"

#define BUF_MAX 512

typedef struct acceptor_tcp_t {
    int fd;
    char buf[BUF_MAX];
} acceptor_tcp;

void acceptor_tcp_destructor(acceptor_tcp *self) {
    close(self->fd);
    free(self);
}

int acceptor_tcp_send(
        acceptor_tcp *self,
        char *msg,
        size_t nbytes
) {
    if (write(self->fd, msg, nbytes) < 0) return -1;
    return 0;
}

char *acceptor_tcp_recv(
        acceptor_tcp *self,
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

int acceptor_tcp_buf_size() {
    return BUF_MAX;
}

int acceptor_from_tcp(
        acceptor *self,
        int fd
) {
    acceptor_destructor(self);

    acceptor_tcp *p = malloc(sizeof(acceptor_tcp));
    memset(p, 0, sizeof(acceptor_tcp));

    p->fd = fd;

    self->p = p;
    self->destructor = (void (*)(void *)) &acceptor_tcp_destructor;
    self->send_func = (int (*)(void *, char *, size_t)) &acceptor_tcp_send;
    self->recv_func = (char *(*)(void *, size_t)) &acceptor_tcp_recv;
    self->buffer_size = &acceptor_tcp_buf_size;

    return 0;
}

typedef struct listener_tcp_t {
    int fd;
} listener_tcp;

void listener_tcp_destructor(listener_tcp *self) {
    close(self->fd);
    free(self);
}

int listener_tcp_next_client(listener_tcp *self, acceptor *c) {
    struct sockaddr_storage cli_addr;
    socklen_t cli_addr_len = sizeof(cli_addr);

    int conn_fd = accept(
            self->fd,
            (struct sockaddr *) &cli_addr,
            &cli_addr_len
    );
    if (conn_fd < 0) return 1;

    acceptor_from_tcp(c, conn_fd);

    return 0;
}

int listen_to_tcp(struct sockaddr_storage addr) {
    int sock_fd = socket(addr.ss_family, SOCK_STREAM, 0);
    if (sock_fd < 0) return -1;

    int enable_reuse = 1;
    if (setsockopt(
            sock_fd,
            SOL_SOCKET, SO_REUSEPORT,
            &enable_reuse, sizeof(int)
    )) return -1;

    if (bind(sock_fd, (struct sockaddr *) &addr, sizeof(addr))) return -1;
    if (listen(sock_fd, 5)) return -1;

    return sock_fd;
}

int listener_from_tcp(listener *self, struct sockaddr_storage addr) {
    listener_destructor(self);

    int sock_fd = listen_to_tcp(addr);
    if (sock_fd < 0) return 1;

    listener_tcp *p = malloc(sizeof(listener_tcp));
    p->fd = sock_fd;

    self->p = p;
    self->destructor = (void (*)(void *)) &listener_tcp_destructor;
    self->next_client = (int (*)(void *, acceptor *)) &listener_tcp_next_client;

    return 0;
}

