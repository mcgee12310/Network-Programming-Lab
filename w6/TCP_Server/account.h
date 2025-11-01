#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#define MAX_USERNAME_LENGTH 1000
#define MAX_MESSAGE_LENGTH 2000
#define MAX_ACCOUNTS 1000000

#define BLANK_STR ""

typedef struct
{
    char username[MAX_USERNAME_LENGTH];
    int status; // 1: active, 0: banned
    bool is_logged_in;
} Account;

extern Account accounts[MAX_ACCOUNTS];
extern char current_user[MAX_USERNAME_LENGTH];
extern int account_number;

void load_accounts(const char *filename, Account accounts[]);
int authorize_user(char *log_in_username);
void log_out(char *current_user);
void post_message();

#endif // ACCOUNT_H
