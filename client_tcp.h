#pragma once

#include <netinet/in.h>

typedef struct connector_t connector;

// Initialize a TCP `connector` by connecting to the server at `addr`.
// Replaces `self` with the new TCP connector.
int connector_from_tcp(connector *self, struct sockaddr_storage addr);
