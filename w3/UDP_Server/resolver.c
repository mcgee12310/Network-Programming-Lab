#include "resolver.h"

#define NOT_FOUND_MSG "-Information not found"
#define BUFFER_SIZE 8193

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
int is_valid_domain(const char *domain)
{
    int len = strlen(domain);
    if (len == 0 || len > 255)
        return 0;

    int label_len = 0;
    int has_dot = 0;

    for (int i = 0; i < len; i++)
    {
        char c = domain[i];

        if (c == '.')
        {
            if (label_len == 0)
                return 0;
            if (domain[i - 1] == '-')
                return 0;
            label_len = 0;
            has_dot = 1;
        }
        else
        {
            if (!(isalnum(c) || c == '-'))
                return 0;
            label_len++;
            if (label_len > 63)
                return 0;
        }
    }

    if (domain[len - 1] == '-')
        return 0;
    if (!has_dot)
        return 0;

    return 1;
}

/**
 * This function resolves the given IPv4 address to its corresponding hostname
 * and prints the result
 * @param ip The IPv4 address to resolve
 * @param reply_data Buffer to store the resolved hostname or "Not found information" if resolution fails
 */
void resolve_ip(const char *ip, char reply_data[BUFFER_SIZE])
{
    struct sockaddr_in addr;
    struct hostent *ret;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);

    ret = gethostbyaddr(&addr.sin_addr, 4, AF_INET);
    if (ret == NULL){
        strncpy(reply_data, NOT_FOUND_MSG, BUFFER_SIZE-1);
        reply_data[BUFFER_SIZE-1] = '\0';
    }
    else
    {
        reply_data[0] = '+';
        strcat(reply_data, ret->h_name);
    }
}

/**
 * This function resolves the given domain name to its corresponding IPv4 addresses
 * and prints the results
 * @param domain The domain name to resolve
 * @param reply_data Buffer to store the resolved IPv4 addresses or "Not found information" if resolution fails
 */
void resolve_domain(const char *domain, char reply_data[])
{
    struct addrinfo *result, *p, hints;
    int error;
    char ipStr[INET_ADDRSTRLEN];
    size_t total_len = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo(domain, NULL, &hints, &result);
    if (error != 0)
    {
        strncpy(reply_data, NOT_FOUND_MSG, BUFFER_SIZE - 1);
        reply_data[BUFFER_SIZE - 1] = '\0';
        return;
    }

    reply_data[0] = '+';
    reply_data[1] = '\0';
    total_len = 1;
    for (p = result; p != NULL; p = p->ai_next)
    {
        struct sockaddr_in *addr = (struct sockaddr_in *)p->ai_addr;
        inet_ntop(AF_INET, &addr->sin_addr, ipStr, sizeof(ipStr));

        size_t ip_len = strlen(ipStr);
        if (total_len + ip_len + 2 >= BUFFER_SIZE)
            break;

        if (total_len > 1)
        {
            reply_data[total_len++] = ' ';
            reply_data[total_len] = '\0';
        }

        strcat(reply_data, ipStr);
        total_len += ip_len;
    }

    freeaddrinfo(result);

    if (total_len == 0)
        strncpy(reply_data, NOT_FOUND_MSG, BUFFER_SIZE - 1);
}
