#include "util.h"

#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int parse_int(const char* str, int* res) {
    char* ptr;
    int _res = strtol(str, &ptr, 10);
    if (ptr == str) { // no number?
        return -1;
    }
    if (ptr < str + strlen(str)) { // partial number
        return -2;
    }
    *res = _res;
    
    return 0;
}

int wildcard_addr(int port, struct sockaddr_storage* addr) {
    addr->ss_family = AF_INET;
    struct sockaddr_in* _addr = (struct sockaddr_in*) addr;
    _addr->sin_addr.s_addr = htonl(INADDR_ANY);
    _addr->sin_port = htons(port);

    return 0;
}

int wildcard_addr6(int port, struct sockaddr_storage* addr) {
    addr->ss_family = AF_INET6;
    struct sockaddr_in6* _addr = (struct sockaddr_in6*) addr;
    _addr->sin6_addr = in6addr_any;
    _addr->sin6_port = htons(port);

    return 0;
}

int lookup_addr(char* addr_str, int port, struct sockaddr_storage* addr) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    struct addrinfo* addr_info;
    if (getaddrinfo(addr_str, NULL, &hints, &addr_info)) {
        return 1;
    }
    *addr = *((struct sockaddr_storage*) addr_info->ai_addr);
    if (addr->ss_family == AF_INET6) {
        struct sockaddr_in6* _addr = (struct sockaddr_in6*) addr;
        _addr->sin6_port = htons(port);
    } else if (addr->ss_family == AF_INET) {
        struct sockaddr_in* _addr = (struct sockaddr_in*) addr;
        _addr->sin_port = htons(port);
    }

    freeaddrinfo(addr_info);

    return 0;
}

char addr6_str[INET6_ADDRSTRLEN];
char* print_addr6(struct sockaddr_in6* addr) {
    inet_ntop(
        addr->sin6_family,
        &(addr->sin6_addr),
        addr6_str, INET6_ADDRSTRLEN);
    return addr6_str;
}

char addr_str[INET_ADDRSTRLEN];
char* print_addr(struct sockaddr_storage* addr) {
    if (addr->ss_family == AF_INET6) {
        return print_addr6((struct sockaddr_in6*) addr);
    }

    struct sockaddr_in* _addr = (struct sockaddr_in*) addr;
    inet_ntop(
        _addr->sin_family,
        &(_addr->sin_addr),
        addr_str, INET_ADDRSTRLEN);

    return addr_str;
}
