#include "server.h"
#include "server_tcp.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>

#include "third_party/log.h"

// Parsed command line arguments
struct args_t {
    int port;
} args;

// Parse command line arguments
int parse_args(int argc, char *argv[]) {
    if (argc > 3) {
        log_fatal("Usage: server_c_tcp <port>");
        return 1;
    }

    if (argc == 2) {
        log_trace("args: %s", argv[1]);
        if (parse_int(argv[1], &args.port) < 0) {
            log_fatal("<port> is not a number");
            return 1;
        }
    } else if (argc == 3) {
        log_trace("args: %s, %s", argv[1], argv[2]);
        if (parse_int(argv[1], &args.port) < 0) { // TODO: Gradescope fails here
            log_fatal("<port> is not a number");
            return 1;
        }
    } else if (argc == 1) {
        args.port = 8080;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    {
        FILE *fd = fopen("hangman_server.log", "w");
        log_add_fp(fd, LOG_TRACE);
    }

    srand(0);

    if (parse_args(argc, argv)) return 1;

    struct sockaddr_storage addr;
    if (wildcard_addr(args.port, &addr)) {
        log_fatal("Error creating wildcard addr");
        return 1;
    }

    log_info(
            "server starting at addr: %s, port: %d",
            print_addr(&addr),
            args.port
    );

    server *s; // server interface
    {
        listener l_tcp; // TCP backend listener
        listener_new(&l_tcp);
        if (listener_from_tcp(&l_tcp, addr)) {
            log_fatal("binding to addr %s:%d failed", print_addr(&addr), args.port);
            return 1;
        }

        s = server_new(l_tcp);
        if (!s) { // unknown error (e.g. malloc failure)
            log_fatal("server creation failed");
            return 1;
        }
    }

    server_run(s);

    // cleanup
    server_destructor(s);
    free(s);
}