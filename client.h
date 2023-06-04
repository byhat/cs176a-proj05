#pragma once

#include <netinet/in.h>

// `connector` interface to exchange messages with the server.
// `client` uses this to connect and exchange data with the server.
//
// Implementation detail:
// - Runtime-level polymorphism using vtable.
typedef struct connector_t {
    void *p; // pointer to implementation
    void (*destructor)(void *); // virtual destructor

    // virtual methods
    int (*send_func)(void *, const char *, size_t);
    const char *(*recv_func)(void *, size_t);

    int (*buffer_size)();
} connector;

// Create a new `connector` interface.
//
// Arguments:
// - `self`: pointer to allocated memory to store the new `connector`.
//
// Protocol-specific implementations can be found in
/// client_tcp.c and client_udp.c.
void connector_new(connector *self);

// Destroy an existing `connector`. free() is *not* called on `self`.
// Usually unnecessary, as `client` will take ownership of it.
//
// Arguments:
// - `self`: pointer to `connector` to destroy.
void connector_destructor(connector *self);

// `client` logic implementation, hidden from the user.
typedef struct client_impl_t client_impl;

// `client` interface
// Provides a high-level interface for interacting with the server.
typedef struct client_t {
    connector c; // protocol-specific `connector`
    client_impl *p; // pointer to actual object, handles client logic
} client;

// Create a new `client` from connector.
// The `connector` must be initialized (connection established).
// Afterwards, the client takes ownership of it.
//
// Arguments:
// - `c`: protocol-specific `connector` backend of the client.
//
// Returns the new `client` on success, NULL on failure.
client* client_new(connector c);

// Destroy an existing `client` (and the `connector` it contains).
// free() is *not* called on `self`.
//
// Arguments:
// - `self`: pointer to `client` to destroy.
void client_destructor(client *self);

// Run the `client`.
//
// Arguments:
// - `self`: pointer to `client`.
// - `input`: input to send to the server.
void client_run(client *self);
