#pragma once

#include <netinet/in.h>

// `acceptor` interface to exchange messages with a single client.
//
// Implementation detail:
// - Runtime-level polymorphism using vtable.
typedef struct acceptor_t {
    void *p; // pointer to implementation
    void (*destructor)(void *); // virtual destructor

    // virtual methods
    int (*send_func)(void *, char *, size_t);
    char *(*recv_func)(void *, size_t);

    int (*buffer_size)();
} acceptor;

// Create a new `acceptor` interface.
//
// Arguments:
// - `self`: pointer to allocated memory to store the new `acceptor`.
//
// Protocol-specific implementations can be found in
/// server_tcp.c and server_udp.c.
void acceptor_new(acceptor *self);

// Destroy an existing `acceptor`. free() is *not* called on `self`.
// Usually unnecessary, as `listener` will take ownership of it.
//
// Arguments:
// - `self`: pointer to `acceptor` to destroy.
void acceptor_destructor(acceptor *self);

// `listener` interface to handle incoming clients.
//
// Implementation detail:
// - Runtime-level polymorphism using vtable.
typedef struct listener_t {
    void *p; // pointer to implementation
    void (*destructor)(void *); // virtual destructor

    int (*next_client)(void *, acceptor *);
} listener;

// Create a new `listener` interface.
//
// Arguments:
// - `self`: pointer to allocated memory to store the new `listener`.
//
// Protocol-specific implementations can be found in
/// server_tcp.c and server_udp.c.
void listener_new(listener *self);

// Destroy an existing `listener`. free() is *not* called on `self`.
// Usually unnecessary, as `server` will take ownership of it.
//
// Arguments:
// - `self`: pointer to `listener` to destroy.
void listener_destructor(listener *self);

// `server` logic implementation, hidden from the user.
typedef struct server_impl_t server_impl;

// `server` interface
// Provides a high-level interface for handling clients.
typedef struct server_t {
    listener l; // protocol-specific `listener`
    server_impl *p; // pointer to actual object, handles server logic
} server;

// Create a new `server` from connector.
// The `listener` must be initialized (address bound).
// Afterwards, the server takes ownership of it.
//
// Arguments:
// - `l`: protocol-specific `listener` backend of the server.
//
// Returns the new `server` on success, NULL on failure.
server *server_new(listener l);

// Destroy an existing `server` (and the `listener` it contains).
// free() is *not* called on `self`.
//
// Arguments:
// - `self`: pointer to `server` to destroy.
void server_destructor(server *self);

// Run the `server`.
//
// Arguments:
// - `self`: pointer to `server`.
void server_run(server *self);
