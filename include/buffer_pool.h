#ifndef BUFFER_POOL_H       
#define BUFFER_POOL_H 

#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h> 
#include "bank.h"

#define BUFFER_POOL_SIZE 5

typedef struct {
    int account_id;
    Account *data;
    bool in_use;
} BufferSlot;

typedef struct {
    BufferSlot slots[BUFFER_POOL_SIZE];
    sem_t empty_slots;
    sem_t full_slots;
    pthread_mutex_t pool_lock;

    int total_loads; 
    int total_unloads;
    int peak_usage; 
    int current_usage;
    int blocked_ops; 
} BufferPool;

extern BufferPool buffer_pool;
 
void buffer_pool_init(void);
void buffer_pool_destroy(void);
void buffer_pool_load(int account_id);
void buffer_pool_unload(int account_id);
void buffer_pool_print_stats(void);

#endif