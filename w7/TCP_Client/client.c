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
#define CHUNK_SIZE 65536
#define MAX_CMD_LENGTH 1000

int client_sock;
char buff[BUFF_SIZE + 1];
char mess[CHUNK_SIZE];
struct sockaddr_in server_addr; /* server's address information */
int sent_bytes, received_bytes;
char cmd[MAX_CMD_LENGTH];

/**
 * @brief Handles and displays the server's response message. Eliminates the status code prefix.
 * @param response The response message from the server.
 */
void handle_server_response(char *buffer)
{
    char *line = strtok(buffer, "\r\n"); // tách từng dòng bằng CRLF
    while (line != NULL)
    {
        printf("\n=> Server response: ");
        line[strcspn(line, "\r\n")] = '\0'; // Remove trailing \r\n

        if (strcmp(line, "100") == 0)
        {
            printf("Connected to server successfully!\n");
        }
        else if (strcmp(line, "110") == 0)
        {
            printf("Logged in successfully!\n");
        }
        else if (strcmp(line, "211") == 0)
        {
            printf("Account is banned. Login failed.\n");
        }
        else if (strcmp(line, "212") == 0)
        {
            printf("Unknown account. Login failed.\n");
        }
        else if (strcmp(line, "213") == 0)
        {
            printf("You have already logged in.\n");
        }
        else if (strcmp(line, "214") == 0)
        {
            printf("Logged in from another location.\n");
        }
        else if (strcmp(line, "130") == 0)
        {
            printf("Logged out successfully!\n");
        }
        else if (strcmp(line, "221") == 0)
        {
            printf("You are not logged in.\n");
        }
        else if (strcmp(line, "120") == 0)
        {
            printf("Message posted successfully!\n");
        }
        else if (strcmp(line, "300") == 0)
        {
            printf("Unknown request.\n");
        }
        else
        {
            printf("Unknown error occurred.\n");
        }

        line = strtok(NULL, "\r\n");
    }
}

/**
 * @brief Receives the server's response message.
 * Reads data until the end-of-message sequence "\r\n" is detected.
 * @param mess The buffer to store the complete response message.
 * @param buff The temporary buffer for receiving data chunks.
 */
void receive_response()
{
    memset(mess, 0, sizeof(mess));
    memset(buff, 0, sizeof(buff));

    while (strstr(mess, "\r\n") == NULL)
    {
        ssize_t bytes = recv(client_sock, buff, sizeof(buff), 0);
        if (bytes < 0)
        {
            perror("recv() error");
            break;
        }
        else if (bytes == 0)
        {
            printf("Connection closed by server.\n");
            break;
        }
        strcat(mess, buff);
    }
    handle_server_response(mess);
};

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

    receive_response();
}

/**
 * @brief Send login request to server.
 * Request message format: "USER <username>\r\n"
 * @param username The username to log in.
 */
void log_in()
{
    char log_in_username[MAX_CMD_LENGTH];
    printf("Enter username: ");
    if (!fgets(log_in_username, sizeof(log_in_username), stdin))
    {
        return;
    }
    log_in_username[strcspn(log_in_username, "\n")] = '\0';

    // Send login request to server
    snprintf(buff, sizeof(buff), "USER %s\r\n", log_in_username);
    sent_bytes = send(client_sock, buff, strlen(buff), 0);
    if (sent_bytes < 0)
    {
        perror("send() error");
        return;
    }

    receive_response();
}

/**
 * @brief Send logout request to server.
 * Request message format: "BYE\r\n"
 */
void log_out()
{
    // Send logout request to server
    snprintf(buff, sizeof(buff), "BYE\r\n");
    sent_bytes = send(client_sock, buff, strlen(buff), 0);
    if (sent_bytes < 0)
    {
        perror("send() error");
        return;
    }

    receive_response();
}

/**
 * @brief Send post message request to server.
 * Request message format: "POST <message>\r\n"
 * @param message The message to post.
 */
void post_message()
{
    char message[MAX_CMD_LENGTH];
    printf("Enter message to post: ");
    if (!fgets(message, sizeof(message), stdin))
    {
        return;
    }
    message[strcspn(message, "\n")] = '\0';

    // Send post message request to server
    snprintf(buff, sizeof(buff), "POST %s\r\n", message);
    sent_bytes = send(client_sock, buff, strlen(buff), 0);
    if (sent_bytes < 0)
    {
        perror("send() error");
        return;
    }

    receive_response();
}

void test_tcp()
{
    // This is a placeholder for potential test code.
    char test_msg[] = "USER test_user\r\nPOST Hello, World!\r\nBYE\r\n";
    sent_bytes = send(client_sock, test_msg, strlen(test_msg), 0);
    if (sent_bytes < 0)
    {
        perror("send() error");
        return;
    }

    receive_response();
}

/**
 * @brief Communicates with the server by displaying a menu and handling user commands.
 * @param cmd The command input buffer.
 */
void communicate()
{
    // Step 4: Communicate with server
    while (1)
    {
        printf("\n===== MENU =====\n");
        printf("1. Log in\n");
        printf("2. Post message\n");
        printf("3. Logout\n");
        printf("4. Exit\n");
        printf("================\n");
        printf("Enter command: ");

        if (!fgets(cmd, sizeof(cmd), stdin))
        {
            continue;
        }

        int choice;
        if (sscanf(cmd, "%d", &choice) != 1)
            continue;

        switch (choice)
        {
        case 1:
            log_in();
            break;

        case 2:
            post_message();
            break;

        case 3:
            log_out();
            break;

        case 4:
            printf("Goodbye...\n");
            return;

        case 5:
            test_tcp();
            break;

        default:
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
