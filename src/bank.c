#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bank.h"

Bank bank;

void bank_init(void) {
    bank.num_accounts = 0;
    pthread_mutex_init(&bank.bank_lock, NULL);

    for (int i =0; i < MAX_ACCOUNTS; i++){
        bank.accounts[i].account_id       = -1;
        bank.accounts[i].balance_centavos = 0;
        pthread_rwlock_init(&bank.accounts[i].lock, NULL);

    }
}

void bank_destroy(void) {
    for (int i = 0; i < MAX_ACCOUNTS; i++){
        pthread_rwlock_destroy(&bank.accounts[i].lock);
    }

    pthread_mutex_destroy(&bank.bank_lock);
}

int bank_add_account(int account_id, int initial_balance_centavos) {
    pthread_mutex_lock(&bank.bank_lock);
    
    if (bank.num_accounts >= MAX_ACCOUNTS) {
        fprintf(stderr, "[BANK] ERROR: max accounts (%d) reached\n", MAX_ACCOUNTS);
        pthread_mutex_unlock(&bank.bank_lock);
        return -1;
    }

    for (int i = 0; i < bank.num_accounts; i++) {
        if (bank.accounts[i].account_id == account_id) {
            fprintf(stderr, "[BANK] ERROR: account with id %d already exists\n", account_id);
            pthread_mutex_unlock(&bank.bank_lock);
            return -1;
        }
    }

    int idx = bank.num_accounts;
    bank.accounts[idx].account_id = account_id;
    bank.accounts[idx].balance_centavos = initial_balance_centavos;
    bank.num_accounts++;

    pthread_mutex_unlock(&bank.bank_lock);
    return 0;
}

int bank_find_account(int account_id) {
    pthread_mutex_lock(&bank.bank_lock);

    int found = -1;
    for (int i = 0; i < bank.num_accounts; i++) {
        if (bank.accounts[i].account_id == account_id) {
            found = i;
            break;
        }
    }
    pthread_mutex_unlock(&bank.bank_lock);
    return found;
}

int get_balance(int account_id) {
    int idx = bank_find_account(account_id);

    if (idx == -1) {
        fprintf(stderr, "[BANK] ERROR: account with id %d not found\n", account_id);
        return -1;
    }

    Account *acc = &bank.accounts[idx];

    pthread_rwlock_rdlock(&acc->lock);
    int balance = acc->balance_centavos;
    pthread_rwlock_unlock(&acc->lock);

    return balance;
}

void deposit(int account_id, int amount_centavos) {
    int idx = bank_find_account(account_id);

    if (idx == -1) {
        fprintf(stderr, "[BANK] ERROR: account with id %d not found\n", account_id);
        return;
    }

    Account *acc = &bank.accounts[idx];

    pthread_rwlock_wrlock(&acc->lock);
    acc->balance_centavos += amount_centavos;
    pthread_rwlock_unlock(&acc->lock);
}

bool withdraw(int account_id, int amount_centavos) {
    int idx = bank_find_account(account_id);

    if (idx == -1) {
        fprintf(stderr, "[BANK] ERROR: account with id %d not found\n", account_id);
        return false;
    }

    Account *acc = &bank.accounts[idx];

    pthread_rwlock_wrlock(&acc->lock);

    if (acc->balance_centavos < amount_centavos) {
        pthread_rwlock_unlock(&acc->lock);
        return false;
    }

    acc->balance_centavos -= amount_centavos;
    pthread_rwlock_unlock(&acc->lock);
    return true;
}

bool transfer(int from_id, int to_id, int amount_centavos) {
    int from_idx = bank_find_account(from_id);
    int to_idx = bank_find_account(to_id);

    if (from_idx == -1 || to_idx == -1) {
        fprintf(stderr, "[BANK] ERROR: from account with id %d not found\n", from_id);
        return false;
    }

    Account *first = (from_idx < to_idx) ? &bank.accounts[from_idx] 
                                            : &bank.accounts[to_idx];
    Account *second = (from_idx < to_idx) ? &bank.accounts[to_idx] 
                                             : &bank.accounts[from_idx];
    
    printf("transfer lock order: %d -> %d\n", first->account_id, second->account_id);

    pthread_rwlock_wrlock(&first->lock);
    pthread_rwlock_wrlock(&second->lock);

    Account *from_acc = &bank.accounts[from_idx];
    Account *to_acc = &bank.accounts[to_idx];

    if (from_acc->balance_centavos < amount_centavos) {
        pthread_rwlock_unlock(&second->lock);
        pthread_rwlock_unlock(&first->lock);
        return false;
    }

    from_acc->balance_centavos -= amount_centavos;
    to_acc->balance_centavos += amount_centavos;
    
    pthread_rwlock_unlock(&second->lock);
    pthread_rwlock_unlock(&first->lock);
    return true;
}

int bank_total_centavos(void) {
    int total = 0;

    for (int i = 0; i < bank.num_accounts; i++) {
        Account *acc = &bank.accounts[i];

        pthread_rwlock_rdlock(&acc->lock);
        total += acc->balance_centavos;
        pthread_rwlock_unlock(&acc->lock);
    }

    return total;
}

void bank_print_balances(void) {
    printf("\n=== Account Balances ===\n");
    
    for (int i = 0; i < bank.num_accounts; i++) {
        pthread_rwlock_rdlock(&bank.accounts[i].lock);
        int bal = bank.accounts[i].balance_centavos;
        pthread_rwlock_unlock(&bank.accounts[i].lock);
 
        printf("  Account %3d : PHP %d.%02d\n",
               bank.accounts[i].account_id,
               bal / 100, bal % 100);
    }
}




