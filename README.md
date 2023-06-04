# cs176a-proj05

## Build Instructions

### Prerequisites

- CMake 3.12 or higher
- A compatible C compiler, tested with Clang

### Building

```sh
mkdir build && cd build
cmake ..
make -j4
```

## Running

```sh
./hangman_server 8080 & # Start server on port 8080
./hangman_client 127.0.0.1 8080 # Connect to server on localhost:8080
```

## Quirks

- The server accepts `<port> <args>` as arguments,
as Gradescope runs `hangman_server` with `./hangman_server 8080 0`.
- The server broadcasts a welcome message to new clients when they connect.
This is done to avoid a race condition when the client has to wait for user input and
the server's potential `server-overloaded` response at the same time.
- RNG is always seeded with `0` to make things predictable.
- The server defaults to "default" word when the word list is empty.
- The client sends an empty message to the server when exiting prematurely,
to avoid the server awaiting a response from the client.
- The server writes all its logs into `hangman_server.log`.

## Third-party Components

- [log.c](https://github.com/rxi/log.c) (MIT License)
