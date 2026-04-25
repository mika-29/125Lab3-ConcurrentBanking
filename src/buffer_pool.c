#include <stdio.h>
#include <stdlib.h>
#include "buffer_pool.h"

BufferPool buffer_pool;

void buffer_pool_init(void) {
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        buffer_pool.slots[i].account_id = -1;
        buffer_pool.slots[i].data       = NULL;
        buffer_pool.slots[i].in_use     = false;
    }

    sem_init(&buffer_pool.empty_slots, 0, BUFFER_POOL_SIZE);
    sem_init(&buffer_pool.full_slots,  0, 0);
    pthread_mutex_init(&buffer_pool.pool_lock, NULL);

    buffer_pool.total_loads   = 0;
    buffer_pool.total_unloads = 0;
    buffer_pool.peak_usage    = 0;
    buffer_pool.current_usage = 0;
    buffer_pool.blocked_ops   = 0;
}

void buffer_pool_destroy(void) {
    sem_destroy(&buffer_pool.empty_slots);
    sem_destroy(&buffer_pool.full_slots);
    pthread_mutex_destroy(&buffer_pool.pool_lock);
}

void buffer_pool_load(int account_id) {                                    //Producer — load an account into the pool.
    pthread_mutex_lock(&buffer_pool.pool_lock);                            // Check: is this account already in the pool? (avoid duplicate load) 
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (buffer_pool.slots[i].in_use &&
            buffer_pool.slots[i].account_id == account_id) {
            pthread_mutex_unlock(&buffer_pool.pool_lock);                  // Already loaded — nothing to do. 
            return;
        }
    }
    pthread_mutex_unlock(&buffer_pool.pool_lock);

    int val;
    sem_getvalue(&buffer_pool.empty_slots, &val);
    if (val == 0) {
        buffer_pool.blocked_ops++;                                        //count how many times we had to wait 
    }
    sem_wait(&buffer_pool.empty_slots);  

    pthread_mutex_lock(&buffer_pool.pool_lock);

    int idx = bank_find_account(account_id);
    if (idx == -1) {
        pthread_mutex_unlock(&buffer_pool.pool_lock);                     //Account does not exist in the bank — release the slot we just took. 
        sem_post(&buffer_pool.empty_slots);
        fprintf(stderr, "[BUFFER] ERROR: account %d not found\n", account_id);
        return;
    }

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (!buffer_pool.slots[i].in_use) {
            buffer_pool.slots[i].account_id = account_id;
            buffer_pool.slots[i].data       = &bank.accounts[idx];
            buffer_pool.slots[i].in_use     = true;

            buffer_pool.total_loads++;
            buffer_pool.current_usage++;
            if (buffer_pool.current_usage > buffer_pool.peak_usage)
                buffer_pool.peak_usage = buffer_pool.current_usage;

            printf("[BUFFER] Loaded account %d into slot %d "
                   "(usage %d/%d)\n",
                   account_id, i,
                   buffer_pool.current_usage, BUFFER_POOL_SIZE);
            break;
        }
    }

    pthread_mutex_unlock(&buffer_pool.pool_lock);
    sem_post(&buffer_pool.full_slots);   
}

void buffer_pool_unload(int account_id) {                               //Consumer — unload an account from the pool (after committing changes to the bank).    
    sem_wait(&buffer_pool.full_slots);   

    pthread_mutex_lock(&buffer_pool.pool_lock);

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (buffer_pool.slots[i].in_use &&
            buffer_pool.slots[i].account_id == account_id) {

            buffer_pool.slots[i].in_use     = false;
            buffer_pool.slots[i].account_id = -1;
            buffer_pool.slots[i].data       = NULL;

            buffer_pool.total_unloads++;
            buffer_pool.current_usage--;

            printf("[BUFFER] Unloaded account %d from slot %d "
                   "(usage %d/%d)\n",
                   account_id, i,
                   buffer_pool.current_usage, BUFFER_POOL_SIZE);
            break;
        }
    }

    pthread_mutex_unlock(&buffer_pool.pool_lock);
    sem_post(&buffer_pool.empty_slots);   
}

void buffer_pool_print_stats(void) {                                   //print buffer pool 
    printf("\n=== Buffer Pool Report ===\n");
    printf("Pool size                      : %d slots\n", BUFFER_POOL_SIZE);
    printf("Total loads                    : %d\n", buffer_pool.total_loads);
    printf("Total unloads                  : %d\n", buffer_pool.total_unloads);
    printf("Peak usage                     : %d slots\n", buffer_pool.peak_usage);
    printf("Blocked operations (pool full) : %d\n", buffer_pool.blocked_ops);
}