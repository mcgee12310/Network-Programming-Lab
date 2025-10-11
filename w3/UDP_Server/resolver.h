#ifndef RESOLVER_H
#define RESOLVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int is_valid_ipv4(const char *ip);
int is_valid_domain(const char *domain);
void resolve_ip(const char *domain, char reply_data[]);
void resolve_domain(const char *domain, char reply_data[]);

#endif
