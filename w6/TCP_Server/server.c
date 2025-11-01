#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>

#include "account.h"

#define BACKLOG 20
#define BUFF_SIZE 4096
#define CHUNK_SIZE 65536
#define ACCOUNT_FILE "TCP_Server/account.txt"

#define CONNECTED_MSG "100\r\n"
#define ACTIVE_ACCOUNT_MSG "110\r\n"
#define BANNED_ACCOUNT_MSG "211\r\n"
#define UNKNOWN_ACCOUNT_MSG "212\r\n"
#define ALREADY_LOGGED_IN_MSG "213\r\n"
#define LOGGED_IN_ELSEWHERE_MSG "214\r\n"
#define LOGOUT_SUCCESS_MSG "130\r\n"
#define NOT_LOGGED_IN_MSG "221\r\n"
#define POST_SUCCESS_MSG "120\r\n"
#define UNKNOWN_REQUEST_MSG "300\r\n"

int listen_sock, conn_sock, *connfd; /* file descriptors */
char *port;
struct sockaddr_in server_addr; /* server's address information */
struct sockaddr_in client_addr; /* client's address information */
pthread_t tid;
socklen_t sin_size;

/**
 * @brief Send response message to client.
 * @param sockfd The connected socket file descriptor.
 * @param msg The message to send.
 */
void respond_to_client(int sockfd, char *msg)
{
    int len = strlen(msg);
    int sent_bytes = send(sockfd, msg, len, 0);
    if (sent_bytes < 0)
    {
        perror("send() error");
    }
    printf("=> Sent to client: %s\n", msg);
}

/**
 * @brief Handle client request based on the received message.
 * @param buff The received message buffer.
 * @param received_bytes The number of bytes received.
 */
bool handle_client_request(int sockfd, char *buff, int received_bytes, bool is_logged_in, char* current_user)
{
    buff[strcspn(buff, "\r\n")] = '\0';
    printf("=> Received from client: %s\n", buff);

    if (strncmp(buff, "USER", 4) == 0)
    {
        if (is_logged_in)
        {
            respond_to_client(sockfd, ALREADY_LOGGED_IN_MSG);
            return is_logged_in;
        }
        else
        {
            char log_in_username[MAX_USERNAME_LENGTH];
            sscanf(buff + 5, "%s", log_in_username);
            int res = authorize_user(log_in_username);
            switch (res)
            {
            case 1:
                respond_to_client(sockfd, ACTIVE_ACCOUNT_MSG);
                strcpy(current_user, log_in_username);
                return true;
            case 0:
                respond_to_client(sockfd, UNKNOWN_ACCOUNT_MSG);
                return is_logged_in;
            case -1:
                respond_to_client(sockfd, BANNED_ACCOUNT_MSG);
                return is_logged_in;
            case -2:
                respond_to_client(sockfd, LOGGED_IN_ELSEWHERE_MSG);
                return is_logged_in;
            default:
                break;
            }
        }
    }
    else if (strncmp(buff, "POST", 4) == 0)
    {
        if (is_logged_in)
        {
            post_message();
            respond_to_client(sockfd, POST_SUCCESS_MSG);
            return is_logged_in;
        }
        else
        {
            respond_to_client(sockfd, NOT_LOGGED_IN_MSG);
            return is_logged_in;
        }
    }
    else if (strncmp(buff, "BYE", 3) == 0)
    {
        if (is_logged_in)
        {
            log_out(current_user);
            strcpy(current_user, "");
            respond_to_client(sockfd, LOGOUT_SUCCESS_MSG);
            return false;
        }
        else
        {
            respond_to_client(sockfd, NOT_LOGGED_IN_MSG);
            return is_logged_in;
        }
    }
    else
    {
        respond_to_client(sockfd, UNKNOWN_REQUEST_MSG);
        return is_logged_in;
    }
}
/**
 * @brief Communicate with the client by receiving requests and sending responses.
 * @param sockfd The connected socket file descriptor.
 */
void *communicate(void *arg)
{
    int sockfd;
    sockfd = *((int *)arg);
    free(arg);

    bool is_logged_in = false;
    char current_user[1000] = "";

    printf("Communicating with client %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    respond_to_client(sockfd, CONNECTED_MSG);

    char recv_buf[BUFF_SIZE]; // bộ đệm tạm thời để nhận dữ liệu từ client
    char mess[CHUNK_SIZE];

    while (1)
    {
        int disconnect = 0;
        memset(recv_buf, 0, sizeof(recv_buf));
        memset(mess, 0, sizeof(mess));
        while (strstr(mess, "\r\n") == NULL)
        {
            ssize_t bytes = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
            if (bytes < 0)
            {
                perror("recv() error");
                disconnect = 1;
                break;
            }
            else if (bytes == 0)
            {
                printf("Connection closed: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                disconnect = 1;
                break;
            }
            strcat(mess, recv_buf);
        }
        if (disconnect == 0)
            is_logged_in = handle_client_request(sockfd, mess, strlen(mess), is_logged_in, current_user);
        else
            break;
    }
    
    strcpy(current_user, "");
    log_out(current_user);
    close(sockfd);
}

/**
 * @brief Sets up the server socket to listen for incoming connections.
 * @param port The port number to bind the server socket.
 * @param server_addr The server address structure.
 */
void setup_socket()
{
    // Step 1: Construct a TCP socket to listen connection request
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }

    // Step 2: Bind address to socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* INADDR_ANY puts your IP address automatically */

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind() error");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Main function to start the TCP server.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Exit status.
 */
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    port = argv[1];
    setup_socket();
    load_accounts(ACCOUNT_FILE, accounts);

    // Step 3: Listen request from client
    if (listen(listen_sock, BACKLOG) == -1)
    {
        perror("listen() error");
        exit(EXIT_FAILURE);
    }

    printf("Server started at port number %d!\n", atoi(port));

    while (1)
    {
        connfd = (int *)malloc(sizeof(int));
        if ((*connfd = accept(listen_sock, (struct sockaddr *)&client_addr, (socklen_t *)&sin_size)) == -1)
        {
            perror("accept() error");
            free(connfd);
            continue;
        }

        int client_port = ntohs(client_addr.sin_port);
        char client_ip[INET_ADDRSTRLEN];

        if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN) == NULL)
        {
            perror("inet_ntop() error");
        }
        else
        {
            printf("You got a connection from %s:%d\n", client_ip, client_port);
        }

        /* Spawn a new thread to handle the client */
        pthread_create(&tid, NULL, &communicate, connfd);
    }

    close(listen_sock);
    return 0;
}
