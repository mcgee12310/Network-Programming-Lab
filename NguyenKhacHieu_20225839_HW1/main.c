#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ACCOUNTS 1000000
#define MAX_USERNAME_LENGTH 500
#define MAX_MESSAGE_LENGTH 2000

#define FILE_ACCOUNT "account.txt"

#define BLANK_STR ""

// account struct used in program
typedef struct
{
    char username[1000];
    int status; // 0 = locked, 1 = active
} Account;

// universal variable
Account accounts[MAX_ACCOUNTS];

char current_user[MAX_USERNAME_LENGTH] = "";

int account_number = 0;
int choice;

// load all accounts from file
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
    printf("%d\n", account_number);
    fclose(f);
}

// find account and check status
void authorize_user()
{
    char log_in_username[MAX_USERNAME_LENGTH];
    printf("Username: ");
    scanf("%s", &log_in_username);

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
            }
            else
            {
                printf("Account is banned\n");
            }
            break;
        }
    }
    if (!found)
    {
        printf("Account is not exist\n");
    }
}

// check if already log in
bool logged_in_user()
{
    if (strlen(current_user) > 0)
    {
        printf("You have already logged in\n");
        return true;
    }
    return false;
}

// log in
void log_in()
{
    if (logged_in_user())
        return;

    authorize_user();
}

// log out
void log_out()
{
    if (logged_in_user())
    {
        if(strcpy(current_user, BLANK_STR))
        printf("Successful log out");
        
    }
    else
    {
        printf("You have not logged in");
    }
}

// post message
void post_message()
{
    char message[MAX_MESSAGE_LENGTH];
    printf("Post message: ");
    scanf("%s", &message);

    if (logged_in_user())
        printf("Successful post");
    else
        printf("You have not logged in");
}

int main()
{
    load_accounts(FILE_ACCOUNT, accounts);

    while (1)
    {
        printf("1. Log in\n");
        printf("2. Post message\n");
        printf("3. Logout\n");
        printf("4. Exit\n");
        scanf("%d", &choice);

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
            printf("Exit\n");
            return 0;

        default:
            break;
        }
    }
}
