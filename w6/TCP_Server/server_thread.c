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

#define PORT 5550      /* Port to open */
#define BACKLOG 20     /* Max number of pending connections */
#define BUFF_SIZE 4096 /* Buffer size */

/* Thread function prototype */
void *echo(void *arg);

int main()
{
    int listenfd, *connfd;
    struct sockaddr_in server_addr; /* server's address information */
    struct sockaddr_in client_addr; /* client's address information */
    int sin_size = sizeof(client_addr);
    pthread_t tid;

    /* Step 1: Create socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }

    /* Step 2: Prepare server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Bind to all available interfaces */

    /* Step 3: Bind address to socket */
    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind() error");
        exit(EXIT_FAILURE);
    }

    /* Step 4: Listen for incoming connections */
    if (listen(listenfd, BACKLOG) == -1)
    {
        perror("listen() error");
        exit(EXIT_FAILURE);
    }

    printf("Server started at port %d.\n", PORT);

    /* Step 5: Accept connections and create threads for each client */
    while (1)
    {
        connfd = (int *)malloc(sizeof(int));
        if ((*connfd = accept(listenfd, (struct sockaddr *)&client_addr, (socklen_t *)&sin_size)) == -1)
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
        pthread_create(&tid, NULL, &echo, connfd);
    }

    close(listenfd);
    return 0;
}

/* Thread function: handles communication with a single client */
void *echo(void *arg)
{
    int sockfd;
    int sent_bytes, received_bytes;
    char buff[BUFF_SIZE + 1];

    /* Get the passed socket descriptor */
    sockfd = *((int *)arg);
    free(arg);

    /* Detach the thread so its resources are automatically reclaimed */
    pthread_detach(pthread_self());

    while (1)
    {
        received_bytes = recv(sockfd, buff, BUFF_SIZE, 0);
        if (received_bytes < 0)
        {
            perror("recv() error");
            break;
        }
        else if (received_bytes == 0)
        {
            printf("Connection closed.\n");
            break;
        }

        buff[received_bytes] = '\0';
        printf("Received: %s\n", buff);

        sent_bytes = send(sockfd, buff, received_bytes, 0); /* Echo back to client */
        if (sent_bytes < 0)
        {
            perror("send() error");
            break;
        }
    }

    close(sockfd);
    return NULL;
}
