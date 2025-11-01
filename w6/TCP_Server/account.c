#include "account.h"

Account accounts[MAX_ACCOUNTS];

int account_number = 0;

pthread_mutex_t account_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    accounts[account_number].is_logged_in = false;
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
  pthread_mutex_lock(&account_mutex);

  bool found = false;
  for (int i = 0; i < account_number; i++)
  {
    if (strcmp(log_in_username, accounts[i].username) == 0)
    {
      found = true;

      if (accounts[i].is_logged_in)
      {
        printf("Account %s is already logged in elsewhere.\n", log_in_username);
        pthread_mutex_unlock(&account_mutex);
        return -2; // already logged in elsewhere
      }
      else if (accounts[i].status == 1)
      {
        accounts[i].is_logged_in = true;
        pthread_mutex_unlock(&account_mutex);
        return 1; // success
      }
      else
      {
        pthread_mutex_unlock(&account_mutex);
        return -1; // account is banned
      }
    }
  }

  if (!found)
  {
    pthread_mutex_unlock(&account_mutex);
    return 0; // account not found
  }
}

/**
 * @brief Log out the current user if logged in.
 */
void log_out(char *username)
{
  pthread_mutex_lock(&account_mutex);
  for (int i = 0; i < account_number; i++)
  {
    if (strcmp(accounts[i].username, username) == 0)
    {
      accounts[i].is_logged_in = false;
      printf("Set false: %s", username);
      break;
    }
  }
  pthread_mutex_unlock(&account_mutex);
}

/**
 * @brief Get message input and handle posting process.
 */
void post_message()
{
  printf("Successful post\n");
}
