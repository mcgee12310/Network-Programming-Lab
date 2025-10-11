#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_ACCOUNTS 1000000
#define MAX_CMD_LENGTH 1000
#define MAX_USERNAME_LENGTH 1000
#define MAX_MESSAGE_LENGTH 2000

#define FILE_ACCOUNT "account.txt"
#define FILE_LOG "log_20225839.txt"

#define BLANK_STR ""
#define RESULT_OK "+OK"
#define RESULT_ERR "-ERR"

/**
 * @brief Account structure to hold username and status.
 */
typedef struct
{
    char username[MAX_USERNAME_LENGTH];
    int status; // 1: active, 0: banned
} Account;

Account accounts[MAX_ACCOUNTS];

char current_user[MAX_USERNAME_LENGTH] = "";

int account_number = 0;

char cmd[MAX_CMD_LENGTH];
int choice;

/**
 * @brief Loads account information from a file into the accounts array.
 * @param filename The name of the file to read account data from.
 * @param accounts The array to store loaded account information.
 */
void load_accounts(const char *filename, Account accounts[])
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        printf("cant open file %s\n", filename);
        return;
    }

    while (fscanf(f, "%s %d", accounts[account_number].username, &accounts[account_number].status) == 2)
    {
        account_number++;
        if (account_number >= MAX_ACCOUNTS)
            break;
    }
    fclose(f);
}

/**
 * @brief Writes user's activities to the log file.
 * @param choice The feature made by the user.
 * @param user_input The user input to the above feature.
 * @param result The result of the feature. +OK or -ERR.
 * @return New log in the log file.
 */
void write_log(const int choice, char *user_input, const char *result)
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
    if (strlen(user_input) > 0)
        user_input[strcspn(user_input, "\n")] = ' ';
    fprintf(f, "%s $ %d $ %s$ %s\n", buffer, choice, user_input, result);
    fclose(f);
}

/**
 * @brief Check account and status, then authorize user.
 * @param log_in_username The username input by the user for login.
 * @return Result of authorization process.
 */
void authorize_user(char *log_in_username)
{
    bool found = false;
    for (int i = 0; i < account_number; i++)
    {
        if (strcmp(log_in_username, accounts[i].username) == 0)
        {
            found = true;
            if (accounts[i].status == 1)
            {
                strcpy(current_user, log_in_username);
                printf("Hello %s\n", log_in_username);
                write_log(1, log_in_username, RESULT_OK);
            }
            else
            {
                printf("Account is banned\n");
                write_log(1, log_in_username, RESULT_ERR);
            }
            break;
        }
    }
    if (!found)
    {
        printf("Account is not exist\n");
        write_log(1, log_in_username, RESULT_ERR);
    }
}

/**
 * @brief Check if a user is currently logged in.
 * @return true if a user is logged in, false otherwise.
 */
bool logged_in_user()
{
    if (strlen(current_user) > 0)
        return true;
    else
        return false;
}

/**
 * @brief Get username input and handle login process.
 * @param log_in_username The username input by the user for login.
 * @return Result of login process.
 */
void log_in()
{
    char log_in_username[MAX_USERNAME_LENGTH];
    printf("Username: ");

    fgets(log_in_username, sizeof(log_in_username), stdin);
    log_in_username[strcspn(log_in_username, "\n")] = '\0';

    if (logged_in_user())
    {
        printf("You have already logged in\n");
        write_log(1, log_in_username, RESULT_ERR);
        return;
    }
    authorize_user(log_in_username);
}

/**
 * @brief Log out the current user if logged in.
 * @return Result of logout process.
 */
void log_out()
{
    if (logged_in_user())
    {
        if (strcpy(current_user, BLANK_STR))
        {
            printf("Successful log out\n");
            write_log(3, BLANK_STR, RESULT_OK);
        }
    }
    else
    {
        printf("You have not logged in\n");
        write_log(3, BLANK_STR, RESULT_ERR);
    }
}

/**
 * @brief Get message input and handle posting process.
 * @param message The message input by the user for posting.
 * @return Result of posting process.
 */
void post_message()
{
    char message[MAX_MESSAGE_LENGTH];
    printf("Post message: ");

    fgets(message, sizeof(message), stdin);

    if (logged_in_user())
    {
        printf("Successful post\n");
        write_log(2, message, RESULT_OK);
    }
    else
    {
        printf("You have not logged in\n");
        write_log(2, message, RESULT_ERR);
    }
}

/**
 * @brief The main function that drives the program, providing a menu for user interaction.
 * @param cmd The command input by the user.
 * @param choice The feature choice made by the user.
 */
int main()
{
    load_accounts(FILE_ACCOUNT, accounts);

    while (1)
    {
        printf("===== MENU =====\n");
        printf("1. Log in\n");
        printf("2. Post message\n");
        printf("3. Logout\n");
        printf("4. Exit\n");

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
            write_log(4, BLANK_STR, RESULT_OK);
            return 0;

        default:
            break;
        }
    }
}
