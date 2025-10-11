// UDP Server
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "resolver.h"

#define FILE_LOG "UDP_Server/log_20225839.txt"
#define BUFFER_SIZE 8193
#define NOT_FOUND_MSG "-Information not found"

int server_sock;
char recv_data[BUFFER_SIZE];
char reply_data[BUFFER_SIZE];
int bytes_sent, bytes_received;
struct sockaddr_in server_addr, client_addr; /* server and client address */
char client_ip_str[INET_ADDRSTRLEN];
int sin_size = sizeof(struct sockaddr_in);

/**
 * @brief Setup UDP socket and server address structure
 * @param port Port number of the server
 */
void setup_socket(char *port)
{
  int serv_port = atoi(port);

  if ((server_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) /* call socket() */
  {
    perror("socket() error: ");
    exit(1);
  }

  memset(&server_addr, 0, sizeof(server_addr)); /* Zero the rest of the structure */
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(serv_port);
  server_addr.sin_addr.s_addr = INADDR_ANY; /* INADDR_ANY puts your IP address automatically */

  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) /* call bind() */
  {
    perror("bind() error: ");
    exit(1);
  }
};

/**
 * @brief Writes server activities to the log file.
 * @return New log in the log file.
 */
void write_log()
{
  time_t now;
  struct tm *t;
  char buffer[100];

  time(&now);
  t = localtime(&now);

  strftime(buffer, sizeof(buffer), "[%d/%m/%Y %H:%M:%S]", t);

  FILE *f = fopen(FILE_LOG, "a");
  if (!f)
  {
    printf("cant open file %s\n", FILE_LOG);
    return;
  }

  fprintf(f, "%s$%s$%s\n", buffer, recv_data, reply_data);
  fclose(f);
}

/**
 * @brief Receive message from client, process it and send reply
 */
void communicate()
{
  bytes_received = recvfrom(server_sock, recv_data, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr, &sin_size);
  if (bytes_received < 0)
  {
    perror("recvfrom() error: ");
    return;
  }
  else
  {
    recv_data[bytes_received] = '\0';
    if (inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip_str, INET_ADDRSTRLEN) == NULL)
    {
      perror("inet_ntop() error: ");
    }
    else
    {
      printf("Received from client [%s:%d]: %s [%d bytes]\n", client_ip_str, ntohs(client_addr.sin_port), recv_data, bytes_received);
    }

    if (is_valid_ipv4(recv_data))
    {
      resolve_ip(recv_data, reply_data);
    }
    else if (!(strspn(recv_data, "0123456789.") == strlen(recv_data)) && is_valid_domain(recv_data))
    {
      resolve_domain(recv_data, reply_data);
    }
    else
    {
      strncpy(reply_data, NOT_FOUND_MSG, BUFFER_SIZE - 1);
    }

    printf("Reply to client: ");
    puts(reply_data);
    bytes_sent = sendto(server_sock, reply_data, strlen(reply_data), 0,
                        (struct sockaddr *)&client_addr, sin_size);
    if (bytes_sent < 0)
    {
      perror("sendto() error: ");
    }
  }
}

/**
 * @brief Main function
 * @param argc Number of command line arguments
 * @param argv Command line arguments: <program> <server_port>
 * @return Exit status
 */
int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    return 1;
  }

  setup_socket(argv[1]);

  while (1)
  {
    communicate();
    write_log();
    memset(&(reply_data), 0, sizeof(reply_data));
  }

  close(server_sock);
  return 0;
}
