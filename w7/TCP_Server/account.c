#include "account.h"

Account accounts[MAX_ACCOUNTS];
char current_user[MAX_USERNAME_LENGTH] = "";
int account_number = 0;

/**
 * @brief Loads account information from a file into the accounts array.
 * @param filename The name of the file containing account information.
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
  printf("Loaded %d accounts from %s\n", account_number, filename);
  fclose(f);
}

/**
 * @brief Check account and status, then authorize user.
 * @param log_in_username The username to authorize.
 * @return 1 if success, 0 if account not found, -1 if account is banned.
 */
int authorize_user(char *log_in_username)
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
        return 1; // success
      }
      else
      {
        return -1; // account is banned
      }
      break;
    }
  }

  if (!found)
  {
    return 0; // account not found
  }
}

/**
 * @brief Check if a user is currently logged in.
 * @return true if a user is logged in, false otherwise.
 */
bool logged_in_user()
{
  return strlen(current_user) > 0;
}

/**
 * @brief Log out the current user if logged in.
 */
void log_out()
{
  if (logged_in_user())
  {
    strcpy(current_user, BLANK_STR);
  }
}

/**
 * @brief Get message input and handle posting process.
 */
void post_message()
{
  if (logged_in_user())
  {
    printf("Successful post\n");
  }
  else
  {
    printf("You have not logged in\n");
  }
}
