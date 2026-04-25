#ifndef BANK_H
#define BANK_H
#include <pthread.h>
#include <stdbool.h>

#define MAX_ACCOUNTS 100

typedef struct {
    int account_id;
    int balance_centavos;
    pthread_rwlock_t lock;      // per-account rwlock
} Account;

typedef struct {
    Account accounts[MAX_ACCOUNTS];
    int num_accounts;
    pthread_mutex_t bank_lock;  // protects bank metadata
} Bank;

extern Bank bank;
void bank_init(void);
void bank_destroy(void);
int bank_add_account(int account_id, int initial_balance_centavos);
int bank_find_account(int account_id);
int get_balance(int account_id);
void deposit(int account_id, int amount_centavos);
bool withdraw (int account_id, int amount_centavos);
bool transfer(int from_id, int to_id, int amount_centavos);
int bank_total_centavos(void);
void bank_print_balances(void);

#endif
