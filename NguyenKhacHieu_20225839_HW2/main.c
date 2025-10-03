#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/**
 * This function checks if the given string is a valid IPv4 address
 * @param ip The input string to check
 * @return 1 if the input string is a valid IPv4 address, 0 otherwise
 */
int is_valid_ipv4(const char *ip)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) == 1;
}

/**
 * This function checks if the given string is a valid domain name
 * @param domain The input string to check
 * @return 1 if the input string is a valid domain name, 0 otherwise
 */
int is_valid_domain(const char *domain) {
    int len = strlen(domain);
    if (len == 0 || len > 253) return 0;

    int label_len = 0;
    int has_dot = 0;

    for (int i = 0; i < len; i++) {
        char c = domain[i];

        if (c == '.') {
            if (label_len == 0) return 0;
            if (domain[i-1] == '-') return 0;
            label_len = 0;
            has_dot = 1;
        } else {
            if (!(isalnum((unsigned char)c) || c == '-')) return 0;
            label_len++;
            if (label_len > 63) return 0;
        }
    }

    if (domain[len-1] == '-') return 0;
    if (!has_dot) return 0;

    return 1;
}

/**
 * @param ip The IPv4 address to resolve
 * This function resolves the given IPv4 address to its corresponding hostname
 * and prints the result
 * @return The resolved hostname or "Not found information" if resolution fails
 */
void resolve_ip(const char *ip)
{
    struct sockaddr_in addr;
    int ret;
    char hostname[NI_MAXHOST];
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);

    ret = getnameinfo((struct sockaddr *)&addr, sizeof(struct sockaddr), hostname, NI_MAXHOST, NULL, 0, 0);
    if (ret != 0)
        printf("Not found information\n");
    else{
        printf("Result:\n");
        printf("%s\n", hostname);
    }
}

/**
 * @param domain The domain name to resolve
 * This function resolves the given domain name to its corresponding IPv4 addresses
 * and prints the results
 * @return The resolved IPv4 addresses or "Not found information" if resolution fails
 */
void resolve_domain(const char *domain)
{
    struct addrinfo *result, *p, hints;
    int error;
    char ipStr[INET_ADDRSTRLEN];

    hints.ai_family = AF_INET;
    hints.ai_socktype = 0;

    error = getaddrinfo(domain, NULL, &hints, &result);
    if (error != 0)
    {
        printf("Not found information\n");
        return;
    }

    printf("Result:\n");
    for (p = result; p != NULL; p = p->ai_next)
    {
        struct sockaddr_in *addr = (struct sockaddr_in *)p->ai_addr;
        inet_ntop(AF_INET, &addr->sin_addr, ipStr, sizeof(ipStr));
        printf("%s\n", ipStr);
    }

    freeaddrinfo(result);
}

/**
 * @param argc The number of command-line arguments
 * @param argv The array of command-line argument strings
 * This is the main function that processes command-line arguments and calls
 * the appropriate functions to resolve IP addresses or domain names
 * @return 0 on success, 1 on failure
 */
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("No information\n");
        return 1;
    }

    char *param = argv[1];

    if (is_valid_ipv4(param))
    {
        resolve_ip(param);
    }
    else if (is_valid_domain(param))
    {
        resolve_domain(param);
    }
    else
    {
        printf("No information\n");
    }

    return 0;
}
