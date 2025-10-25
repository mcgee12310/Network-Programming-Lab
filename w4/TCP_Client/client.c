/* TCP Client */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUFF_SIZE 4096
#define CHUNK_SIZE 65536 // 64KB

int client_sock;
char buff[BUFF_SIZE + 1];
struct sockaddr_in server_addr; /* server's address information */
int msg_len, sent_bytes, received_bytes;
char filepath[BUFF_SIZE];
unsigned long filesize;

/**
 * @brief Handles and displays the server's response message. Eliminates the status code prefix.
 * @param response The response message from the server.
 */
void handle_server_response()
{
    buff[received_bytes] = '\0';
    char *msg = buff;
    if (strncmp(buff, "+OK", 3) == 0)
    {
        msg += 3; // eliminate +OK
    }
    else if (strncmp(buff, "-ERR", 4) == 0)
    {
        msg += 4; // eliminate -ERR
    }

    printf("Server response: %s\n", msg);
}

/**
 * @brief Sets up the client socket and connects to the server.
 * @param ip The server's IP address.
 * @param port The server's port number.
 */
void setup_socket(char *ip, char *port)
{
    // Step 1: Construct socket
    int client_port = atoi(port);
    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }

    // Step 2: Specify server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(client_port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // Step 3: Request to connect server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
    {
        perror("connect() error");
        exit(EXIT_FAILURE);
    }

    received_bytes = recv(client_sock, buff, BUFF_SIZE, 0);
    if (received_bytes < 0)
    {
        perror("recv() error");
    }
    else if (received_bytes == 0)
    {
        printf("Connection closed.\n");
    }
    else
    {
        handle_server_response();
    }
}

/**
 * @brief Communicates with the server to send files. Enter file paths to send files.
 * File's size is determined automatically. Blank input exits the program.
 * @param filename The name of the file to be sent.
 */
void communicate()
{
    // Step 4: Communicate with server
    while (1)
    {
        printf("\nEnter file's name: ");
        if (fgets(buff, BUFF_SIZE, stdin) == NULL)
        {
            break;
        }

        buff[strcspn(buff, "\n")] = 0;

        if (strlen(buff) == 0)
        {
            printf("Goodbye!\n");
            break;
        }

        // set filepath
        strcpy(filepath, buff);

        // get filename from filepath
        char *filename = strrchr(filepath, '/');
        if (filename)
            filename++;
        else
            filename = filepath;

        // get filesize
        struct stat file_stat;
        if (stat(filepath, &file_stat) < 0) // check file info
        {
            perror("stat() error");
            continue;
        }
        filesize = file_stat.st_size; // get file's size (bytes)

        // open file to read
        FILE *fp = fopen(filepath, "rb");
        if (fp == NULL)
        {
            perror("fopen() error");
            continue;
        }

        // send request to server
        sprintf(buff, "UPLD %s %lu", filename, filesize);
        send(client_sock, buff, strlen(buff), 0);

        received_bytes = recv(client_sock, buff, sizeof(buff) - 1, 0);
        if (received_bytes <= 0)
        {
            perror("recv() error (please send)");
            fclose(fp);
            break;
        }

        buff[received_bytes] = '\0';
        if (buff[0] == '-')
        {
            printf("Server error: %s", buff);
            fclose(fp);
            continue;
        }
        else if (buff[0] == '+')
        {
            handle_server_response();
            puts("Start sending file...");
            size_t total_sent = 0;
            char send_data[CHUNK_SIZE];
            while (1)
            {
                size_t bytes_read = fread(send_data, 1, sizeof(send_data), fp);
                if (bytes_read == 0)
                    break; // hết file hoặc lỗi

                size_t sent = 0;
                while (sent < bytes_read)
                {
                    ssize_t sent_bytes = send(client_sock, send_data + sent, bytes_read - sent, 0); // ko dung  strlen(buff) tai vi file nhi phan, buff se chi doc den /0, dung bytes_read - sent de gui chinh xac so byte con lai
                    if (sent_bytes < 0)
                    {
                        perror("send() error");
                        break;
                    }
                    sent += sent_bytes;
                    total_sent += sent_bytes;
                }
            }
            printf("File '%s' sent (%lu/%lu bytes)\n", filename, total_sent, filesize);
            fclose(fp);
        }
        else
        {
            printf("Unexpected server response: %s", buff);
            fclose(fp);
            continue;
        }

        // receive server confirmation
        received_bytes = recv(client_sock, buff, sizeof(buff) - 1, 0);
        if (received_bytes > 0)
        {
            handle_server_response();
        }
        else
        {
            perror("recv() error (final)");
            break;
        }
    }
}

/**
 * @brief Main function to start the TCP client.
 * @param argc Argument count.
 * @param argv Command line arguments: <program> <server_ip> <server_port>
 * @return Exit status.
 */
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        return 1;
    }

    setup_socket(argv[1], argv[2]);

    communicate();

    // Step 5: Close socket
    close(client_sock);
    return 0;
}
