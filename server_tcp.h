#pragma once

#include <netinet/in.h>

typedef struct acceptor_t acceptor;

// Initialize a TCP `acceptor` with the file descriptor `fd`.
// Replaces `self` with the new TCP acceptor.
int acceptor_from_tcp(acceptor *self, int fd);

typedef struct listener_t listener;

// Initialize a TCP `listener` by binding to `addr`.
// Replaces `self` with the new TCP listener.
int listener_from_tcp(listener *self, struct sockaddr_storage addr);
