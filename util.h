#pragma once

#include <netinet/in.h>

// Parse a string as an integer.
// Returns 0 on success.
int parse_int(const char* str, int* res);

// Fetches the wildcard IPv4 address for the given port.
// e.g. [::1]:1234
// Replaces `addr` with the new address.
// Returns 0 on success.
int wildcard_addr(int port, struct sockaddr_storage* addr);

// Fetches the wildcard IPv6 address for the given port.
// e.g. [::1]:1234
// Replaces `addr` with the new address.
// Returns 0 on success.
int wildcard_addr6(int port, struct sockaddr_storage* addr);

// Fetches the address for the given address_str+port combination.
// e.g. localhost:1234 => [::1]:1234
// Replaces `addr` with the new address.
// Returns 0 on success, 1 on failure.
int lookup_addr(char* addr_str, int port, struct sockaddr_storage* addr);

// Prints the given address to a string.
// Supports IPv4 and IPv6.
char* print_addr(struct sockaddr_storage* addr);
