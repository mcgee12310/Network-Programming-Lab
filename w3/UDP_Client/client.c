// UDP Client
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE 8193
#define NOT_FOUND_MSG "Information not found"

int client_sock;
char buff[BUFF_SIZE];
struct sockaddr_in server_addr;
int sent_bytes, received_bytes;
socklen_t sin_size = sizeof(struct sockaddr);

/**
 * @brief Setup UDP socket and server address structure
 * @param ip IP address of the server
 * @param port Port number of the server
 */
void setup_socket(char *ip, char *port)
{
  int serv_port = atoi(port);

  // Step 1: Construct a UDP socket
  if ((client_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket() error: ");
    exit(1);
  }

  // Step 2: Define the address of the server
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(serv_port);
  server_addr.sin_addr.s_addr = inet_addr(ip);
}

/**
 * @brief Process the reply from the server and print the result
 * Prefix '+' indicates domain name resolution with one or more IPs
 * Prefix '-' indicates IP address resolution with a domain name or not found
 */
void process_reply()
{
  if (buff[0] == '-')
  {
    puts(NOT_FOUND_MSG);
  }
  else if (buff[0] == '+')
  {
    // Bỏ ký tự '+' ở đầu
    const char *ptr = buff + 1;
    char ip[256];

    while (*ptr)
    {
      int i = 0;
      // copy từng từ (IP) cho đến gặp space hoặc '\0'
      while (*ptr && *ptr != ' ')
      {
        ip[i++] = *ptr++;
      }
      ip[i] = '\0';
      printf("%s\n", ip);

      // bỏ qua các space giữa các IP
      while (*ptr == ' ')
        ptr++;
    }
  }
  else
  {
    puts("Invalid response from server");
  }
}

/**
 * @brief Communicate with the server: send request and receive reply
 */
void communicate()
{
  // Step 3: Communicate with server
  sent_bytes = sendto(client_sock, buff, strlen(buff), 0, (struct sockaddr *)&server_addr, sin_size);

  if (sent_bytes < 0)
  {
    perror("sendto () error: ");
  }
  else
  {
    received_bytes = recvfrom(client_sock, buff, BUFF_SIZE - 1, 0, (struct sockaddr *)&server_addr, &sin_size);

    if (received_bytes < 0)
    {
      perror("recvfrom() error: ");
    }
    else
    {
      buff[received_bytes] = '\0';
      process_reply(buff);
    }
  }
}

/**
 * @brief Main function
 * @param argc Number of command line arguments
 * @param argv Command line arguments: <program> <server_ip> <server_port>
 * @return Exit status
 */
int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return 1;
  }

  setup_socket(argv[1], argv[2]);

  while (1)
  {
    printf("Input message: ");
    if (fgets(buff, BUFF_SIZE, stdin) == NULL)
    {
      break; // EOF
    }

    buff[strcspn(buff, "\n")] = 0;

    if (strlen(buff) == 0)
    {
      printf("Goodbye!\n");
      break;
    }

    communicate();
  }

  close(client_sock);
  return 0;
}
