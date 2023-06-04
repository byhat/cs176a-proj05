#include "client.h"
#include "client_tcp.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>

// Parsed command line arguments
struct args_t {
    struct sockaddr_storage addr;
    int port;
} args;

// Parse command line arguments
int parse_args(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: client_c_tcp <host> <port>\n");
        return 1;
    }

    if (parse_int(argv[2], &args.port) < 0) {
        printf("<port> is not a number\n");
        return 1;
    }

    if (lookup_addr(argv[1], args.port, &args.addr)) {
        printf("<host> ip address resolution failure\n");
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
//    printf("---SERVER LOG---\n");
//    FILE *fd = fopen("hangman_server.log", "r");
//    if (fd == NULL) {
//        printf("Server log does not exist\n");
//        exit(0);
//    }
//
//    {
//        char c = fgetc(fd);
//        while (c != EOF) {
//            printf("%c", c);
//            c = fgetc(fd);
//        }
//    }
//    fclose(fd);
//    printf("---END SERVER LOG---\n");
//
//    return 0;

    if (parse_args(argc, argv)) return 1;

//    printf(
//            "client connecting to addr: %s, port: %d\n",
//            print_addr(&args.addr),
//            args.port
//    );

    client *c; // client interface
    {
        connector c_tcp; // TCP backend connector
        connector_new(&c_tcp);
        if (connector_from_tcp(&c_tcp, args.addr)) {
            printf("binding to addr %s:%d failed\n", print_addr(&args.addr), args.port);

            return 1;
        }

        c = client_new(c_tcp);
        if (!c) { // unknown error (e.g. malloc failure)
            printf("client creation failed\n");
            return 1;
        }
    }

    client_run(c);

    // cleanup
    client_destructor(c);
    free(c);
}
