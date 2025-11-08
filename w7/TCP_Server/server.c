#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "account.h"

#define BACKLOG 20
#define BUFF_SIZE 4096
#define MAX_MESS 65536
#define ACCOUNT_FILE "TCP_Server/account.txt"

#define CONNECTED_MSG "100\r\n"
#define ACTIVE_ACCOUNT_MSG "110\r\n"
#define BANNED_ACCOUNT_MSG "211\r\n"
#define UNKNOWN_ACCOUNT_MSG "212\r\n"
#define ALREADY_LOGGED_IN_MSG "213\r\n"
#define LOGOUT_SUCCESS_MSG "130\r\n"
#define NOT_LOGGED_IN_MSG "221\r\n"
#define POST_SUCCESS_MSG "120\r\n"
#define UNKNOWN_REQUEST_MSG "300\r\n"

typedef struct
{
    int sockfd; // socket của client
    // char username[MAX_USERNAME]; // nếu chưa login -> ""
    bool logged_in; // true if logged in, false otherwise
} ClientInfo;

int i, maxi, maxfd, listenfd, connfd, sockfd;
int nready;
ClientInfo client[FD_SETSIZE];
ssize_t ret;
fd_set readfds, allset;
char sendBuff[BUFF_SIZE], rcvBuff[BUFF_SIZE], mess[MAX_MESS];
char *port;
struct sockaddr_in server_addr; /* server's address information */
struct sockaddr_in client_addr; /* client's address information */
socklen_t clilen;

/**
 * @brief Receive message from client.
 * @param sockfd The connected socket file descriptor.
 * @return The length of the received message.
 */
int receive_from_client(int sockfd)
{
    memset(rcvBuff, 0, sizeof(rcvBuff));
    memset(mess, 0, sizeof(mess));
    while (strstr(mess, "\r\n") == NULL)
    {
        ssize_t bytes = recv(sockfd, rcvBuff, sizeof(rcvBuff), 0);
        if (bytes < 0)
        {
            perror("recv() error");
            break;
        }
        else if (bytes == 0)
        {
            printf("Connection closed: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            break;
        }
        strcat(mess, rcvBuff);
    }
    return strlen(mess);
};

/**
 * @brief Send response message to client.
 * @param sockfd The connected socket file descriptor.
 * @param msg The message to send.
 */
int respond_to_client(int sockfd, char *msg)
{
    int len = strlen(msg);
    int sent_bytes = send(sockfd, msg, len, 0);
    printf("=> Sent to client %s:%d: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg);
    return sent_bytes;
}

/**
 * @brief Handle client request based on the received message.
 * @param buff The received message buffer.
 */
void handle_client_request(ClientInfo *client, char *buff)
{
    memset(sendBuff, 0, sizeof(sendBuff));
    char *line = strtok(buff, "\r\n"); // tách từng dòng bằng CRLF
    while (line != NULL)
    {
        line[strcspn(line, "\r\n")] = '\0';
        printf("=> Received from client: %s\n", line);

        if (strncmp(line, "USER", 4) == 0)
        {
            if (client->logged_in)
            {
                strcat(sendBuff, ALREADY_LOGGED_IN_MSG);
            }
            else
            {
                char log_in_username[MAX_USERNAME_LENGTH];
                sscanf(line + 5, "%s", log_in_username);
                int res = authorize_user(log_in_username);
                if (res == 1)
                {
                    strcat(sendBuff, ACTIVE_ACCOUNT_MSG);
                    client->logged_in = true;
                }
                else if (res == 0)
                {
                    strcat(sendBuff, UNKNOWN_ACCOUNT_MSG);
                }
                else if (res == -1)
                {
                    strcat(sendBuff, BANNED_ACCOUNT_MSG);
                }
            }
        }
        else if (strncmp(line, "POST", 4) == 0)
        {
            if (client->logged_in)
            {
                post_message();
                strcat(sendBuff, POST_SUCCESS_MSG);
            }
            else
            {
                strcat(sendBuff, NOT_LOGGED_IN_MSG);
            }
        }
        else if (strncmp(line, "BYE", 3) == 0)
        {
            if (client->logged_in)
            {
                log_out();
                client->logged_in = false;
                strcat(sendBuff, LOGOUT_SUCCESS_MSG);
            }
            else
            {
                strcat(sendBuff, NOT_LOGGED_IN_MSG);
            }
        }
        else
        {
            strcat(sendBuff, UNKNOWN_REQUEST_MSG);
        }
        line = strtok(NULL, "\r\n");
    }
}

/**
 * @brief Communicate with the client by receiving requests and sending responses.
 * @param sockfd The connected socket file descriptor.
 */
void communicate()
{
    while (1)
    {
        maxfd = listenfd; /* initialize */
        maxi = -1;        /* index into client[] array */
        for (i = 0; i < FD_SETSIZE; i++)
            client[i].sockfd = -1; /* -1 indicates available entry */
        FD_ZERO(&allset);
        FD_SET(listenfd, &allset);

        // Step 4: Communicate with clients
        while (1)
        {
            readfds = allset; /* structure assignment */
            nready = select(maxfd + 1, &readfds, NULL, NULL, NULL);
            if (nready < 0)
            {
                perror("select() error: ");
                exit(EXIT_FAILURE);
            }

            if (FD_ISSET(listenfd, &readfds))
            { /* new client connection */
                clilen = sizeof(client_addr);
                if ((connfd = accept(listenfd, (struct sockaddr *)&client_addr, &clilen)) < 0)
                    perror("\nError: ");
                else
                {
                    printf("You got a connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)); /* prints client's IP */
                    for (i = 0; i < FD_SETSIZE; i++)
                        if (client[i].sockfd < 0)
                        {
                            client[i].sockfd = connfd; /* save descriptor */
                            break;
                        }
                    if (i == FD_SETSIZE)
                    {
                        printf("\nToo many clients");
                        close(connfd);
                    }
                    else
                    {
                        FD_SET(connfd, &allset); /* add new descriptor to set */
                        if (connfd > maxfd)
                            maxfd = connfd; /* for select */
                        if (i > maxi)
                            maxi = i; /* max index in client[] array */

                        client[i].logged_in = false; /* set logged_in to false */
                        respond_to_client(connfd, CONNECTED_MSG);
                    }

                    if (--nready == 0)
                        continue; /* no more readable descriptors */
                }
            }

            for (i = 0; i <= maxi; i++)
            { /* check all clients for data */
                if ((sockfd = client[i].sockfd) < 0)
                    continue;
                if (FD_ISSET(sockfd, &readfds))
                {
                    ret = receive_from_client(sockfd);
                    if (ret <= 0)
                    {
                        FD_CLR(sockfd, &allset);
                        close(sockfd);
                        client[i].sockfd = -1;
                    }
                    else
                    {
                        handle_client_request(&client[i], rcvBuff);
                        ret = respond_to_client(sockfd, sendBuff);
                        if (ret < 0)
                        {
                            FD_CLR(sockfd, &allset);
                            close(sockfd);
                            client[i].sockfd = -1;
                            client[i].logged_in = false;
                        }
                    }

                    if (--nready <= 0)
                        break; /* no more readable descriptors */
                }
            }
        }
        return 0;
        close(sockfd);
    }
}

/**
 * @brief Sets up the server socket to listen for incoming connections.
 * @param port The port number to bind the server socket.
 * @param server_addr The server address structure.
 */
void setup_socket()
{
    // Step 1: Construct a TCP socket to listen connection request
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }

    // Step 2: Bind address to socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* INADDR_ANY puts your IP address automatically */

    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
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
    if (listen(listenfd, BACKLOG) == -1)
    {
        perror("listen() error");
        exit(EXIT_FAILURE);
    }

    printf("Server started at port number %d!\n", atoi(port));

    communicate();

    close(listenfd);
    return 0;
}
