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
#include <signal.h>
#include <errno.h>
#include "account.h"

#define BACKLOG 20
#define BUFF_SIZE 4096
#define MAX_MESS 65536
#define ACCOUNT_FILE "TCP_Server/account.txt"

#define CONNECTED_MSG "100"
#define ACTIVE_ACCOUNT_MSG "110"
#define BANNED_ACCOUNT_MSG "211"
#define UNKNOWN_ACCOUNT_MSG "212"
#define ALREADY_LOGGED_IN_MSG "213"
#define LOGOUT_SUCCESS_MSG "130"
#define NOT_LOGGED_IN_MSG "221"
#define POST_SUCCESS_MSG "120"
#define UNKNOWN_REQUEST_MSG "300"

int listen_sock, conn_sock; /* file descriptors */
char *port;
struct sockaddr_in server_addr; /* server's address information */
struct sockaddr_in client_addr; /* client's address information */
pid_t pid;
socklen_t sin_size;

/**
 * @brief Signal handler for SIGCHLD — prevent zombie processes
 * @param signo Signal number
 */
void sig_chld(int signo)
{
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("Child %d terminated\n", pid);
}

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
void handle_client_request(char *buff, int received_bytes)
{
    buff[strcspn(buff, "\n")] = '\0';
    printf("=> Received from client: %s\n", buff);

    if (strncmp(buff, "USER", 4) == 0)
    {
        if (logged_in_user())
        {
            respond_to_client(conn_sock, ALREADY_LOGGED_IN_MSG);
        }
        else
        {
            char log_in_username[MAX_USERNAME_LENGTH];
            sscanf(buff + 5, "%s", log_in_username);
            int res = authorize_user(log_in_username);
            if (res == 1)
            {
                respond_to_client(conn_sock, ACTIVE_ACCOUNT_MSG);
            }
            else if (res == 0)
            {
                respond_to_client(conn_sock, UNKNOWN_ACCOUNT_MSG);
            }
            else if (res == -1)
            {
                respond_to_client(conn_sock, BANNED_ACCOUNT_MSG);
            }
        }
    }
    else if (strncmp(buff, "POST", 4) == 0)
    {
        if (logged_in_user())
        {
            post_message();
            respond_to_client(conn_sock, POST_SUCCESS_MSG);
        }
        else
        {
            respond_to_client(conn_sock, NOT_LOGGED_IN_MSG);
        }
    }
    else if (strncmp(buff, "BYE", 3) == 0)
    {
        if (logged_in_user())
        {
            log_out();
            respond_to_client(conn_sock, LOGOUT_SUCCESS_MSG);
        }
        else
        {
            respond_to_client(conn_sock, NOT_LOGGED_IN_MSG);
        }
    }
    else
    {
        respond_to_client(conn_sock, UNKNOWN_REQUEST_MSG);
    }
}

/**
 * @brief Communicate with the client by receiving requests and sending responses.
 * @param sockfd The connected socket file descriptor.
 */
void communicate(int sockfd)
{
    printf("Communicating with client %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    respond_to_client(sockfd, CONNECTED_MSG);

    char recv_buf[BUFF_SIZE]; // bộ đệm tạm thời để nhận dữ liệu từ client
    // char line_buf[BUFF_SIZE]; // bộ đệm chứa từng dòng đã hoàn chỉnh (đã gặp \r\n)
    // int recv_len = 0;         // số byte hiện đang lưu trong recv_buf
    char mess[MAX_MESS];

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
            handle_client_request(mess, strlen(mess));
        else
            break;
    }

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

    /* Establish a signal handler to catch SIGCHLD */
    signal(SIGCHLD, sig_chld);

    printf("Server started at port number %d!\n", atoi(port));

    while (1)
    {
        sin_size = sizeof(struct sockaddr_in);
        if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &sin_size)) == -1)
        {
            if (errno == EINTR)
                continue;
            else
            {
                perror("accept() error");
                exit(EXIT_FAILURE);
            }
        }

        /* For each client, fork spawns a child, and the child handles the new client */
        pid = fork();

        if (pid == 0) // child process
        {
            int client_port;
            char client_ip[INET_ADDRSTRLEN];

            close(listen_sock);

            if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN) == NULL)
            {
                perror("inet_ntop() error");
                strcpy(client_ip, "unknown");
            }

            client_port = ntohs(client_addr.sin_port);
            printf("You got a connection from %s:%d\n", client_ip, client_port);

            communicate(conn_sock);
            close(conn_sock);
            exit(0);
        }

        /* The parent closes the connected socket since the child handles the new client */
        close(conn_sock);
    }

    close(listen_sock);
    return 0;
}
