/* TCP Server */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define FILE_LOG "TCP_Server/log_20225839.txt"
#define BASE_DIR "TCP_Server"
#define BACKLOG 2        /* Number of allowed connections */
#define BUFF_SIZE 4096   /* Buffer size */
#define CHUNK_SIZE 65536 // 64KB
#define WELCOME_MSG "+OK Welcome to file server"
#define OK_SEND_FILE_MSG "+OK Please send file"
#define OK_SUCCESS_MSG "+OK Successful upload"
#define ERR_INVALID_FILE_MSG "-ERR Invalid file info format"
#define ERR_FILE_INCOMPLETE_MSG "-ERR File transfer incomplete"

int listen_sock, conn_sock; /* file descriptors */
char recv_data[BUFF_SIZE];
char client_ip[INET_ADDRSTRLEN];
int bytes_sent, received_bytes;
socklen_t sin_size;
struct sockaddr_in server_addr; /* server's address information */
struct sockaddr_in client_addr; /* client's address information */
int client_port;

char fullpath[512];
char directory_name[256];
char filename[256];
unsigned long filesize;
char buffer[BUFF_SIZE];

/**
 * @brief Writes server activities to the log file.
 * @return New log in the log file.
 */
void write_log(char *input, char *result)
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

    if (input == NULL)
    {
        fprintf(f, "%s$%s:%d$%s\n", buffer, client_ip, client_port, result);
    }
    else
        fprintf(f, "%s$%s:%d$%s$%s\n", buffer, client_ip, client_port, input, result);
    fclose(f);
}

/**
 * @brief Sets up the server socket.
 * @param port The port number to bind the server socket to.
 */
void setup_socket(char *port)
{
    // Step 1: Construct a TCP socket to listen connection request
    int serv_port = atoi(port);

    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }

    // Step 2: Bind address to socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(serv_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind() error");
        exit(EXIT_FAILURE);
    }

    // Step 3: Listen request from client
    if (listen(listen_sock, BACKLOG) == -1)
    {
        perror("listen() error");
        exit(EXIT_FAILURE);
    }

    snprintf(fullpath, sizeof(fullpath), "%s/%s", BASE_DIR, directory_name); // combine base dir and input dir when start server
    printf("Server started at port %d, saving files in '%s'\n", serv_port, fullpath);

    if (mkdir(fullpath, 0777) == -1) // 7 = 4 + 2 + 1 → rwx for owner, group, other
    {
        if (errno != EEXIST) // if error is not EEXIST exit
        {
            perror("mkdir(fullpath) error");
            close(conn_sock);
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief Communicates with the client to receive files.
 * @param filename The name of the file to be received.
 * @param filesize The size of the file to be received.
 * @return file received successfully or not in the selected folder.
 */
void communicate()
{
    // Step 4: Communicate with client
    while (1)
    {
        // accept request
        sin_size = sizeof(struct sockaddr_in);
        if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &sin_size)) == -1)
        {
            perror("accept() error");
            continue;
        }

        if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN) == NULL)
        {
            perror("inet_ntop() error");
            close(conn_sock);
            continue;
        }

        // send welcome message
        client_port = ntohs(client_addr.sin_port);
        printf("You got a connection from %s:%d\n", client_ip, client_port);
        bytes_sent = send(conn_sock, WELCOME_MSG, strlen(WELCOME_MSG), 0);
        if (bytes_sent < 0)
            perror("send() error");
        write_log(NULL, WELCOME_MSG);

        while (1)
        {

            // receives file name and file data from client
            char recv_data[CHUNK_SIZE];

            received_bytes = recv(conn_sock, recv_data, sizeof(recv_data), 0);
            if (received_bytes <= 0)
            {
                printf("Client %s:%d disconnected\n", client_ip, client_port);
                close(conn_sock);
                break;
            }
            recv_data[received_bytes] = '\0';

            if (sscanf(recv_data, "UPLD %s %lu", filename, &filesize) != 2)
            {
                send(conn_sock, ERR_INVALID_FILE_MSG, strlen(ERR_INVALID_FILE_MSG), 0);
                close(conn_sock);
                write_log(recv_data, ERR_INVALID_FILE_MSG);
                continue;
            }
            else
            {
            	printf("Client %s:%d: %s\n", client_ip, client_port, recv_data);
                send(conn_sock, OK_SEND_FILE_MSG, strlen(OK_SEND_FILE_MSG), 0);
                write_log(recv_data, OK_SEND_FILE_MSG);
            }

            // write file to disk
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", fullpath, filename);
            FILE *fp = fopen(filepath, "wb"); // w = write, b = binary
            if (fp == NULL)
            {
                perror("fopen() error");
                close(conn_sock);
                continue;
            }

            unsigned long total_received = 0;
            while (total_received < filesize)
            {
                received_bytes = recv(conn_sock, buffer, sizeof(buffer), 0);
                if (received_bytes <= 0)
                {
                    break;
                }
                fwrite(buffer, 1, received_bytes, fp);
                total_received += received_bytes;
            }
            fclose(fp);

            if (total_received == filesize)
            {
                send(conn_sock, OK_SUCCESS_MSG, strlen(OK_SUCCESS_MSG), 0);
                printf("File '%s' uploaded successfully (%lu bytes) in '%s'\n", filename, filesize, filepath);
                write_log(recv_data, OK_SUCCESS_MSG);
            }
            else
            {
                send(conn_sock, ERR_FILE_INCOMPLETE_MSG, strlen(ERR_FILE_INCOMPLETE_MSG), 0);
                printf("%s", ERR_FILE_INCOMPLETE_MSG);
                write_log(recv_data, ERR_FILE_INCOMPLETE_MSG);
                close(conn_sock);
            }
        }
    }
}

/**
 * @brief Main function to start the TCP server.
 * @param argc Argument count.
 * @param argv Command line arguments: <program> <server_port> <directory_name>
 * @return Exit status.
 */
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        return 1;
    }

    strcpy(directory_name, argv[2]);

    setup_socket(argv[1]);

    communicate();

    close(listen_sock);
    return 0;
}
