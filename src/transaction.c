#include <stdio.h> 
#include <stdlib.h>
#include "transaction.h"
#include "timer.h"
#include "bank.h"  

void *exec_transac(void *arg) {
    Transaction *tx = (Transaction *)arg;
    wait_for_tick(tx->start_tick);

    pthread_mutex_lock(&tick_lock);
    tx->actual_start = global_tick;
    pthread_mutex_unlock(&tick_lock);

    tx->status = TX_RUNNING;
    printf("Transaction %d started at tick %d\n", tx->tx_id, tx->actual_start);

    for(int i = 0; i < tx->num_ops; i++) {
        Operation *op = &tx->ops[i];

        pthread_mutex_lock(&tick_lock);
        int tick_before = global_tick;
        pthread_mutex_unlock(&tick_lock);

        switch(op->type) {
            case OP_DEPOSIT:
                printf("[T%d] DEPOSIT account %d amount %d centavos\n", tx->tx_id, op->account_id, op->amount_centavos);
                deposit(op->account_id, op->amount_centavos);
                printf("[T%d] DEPOSIT successful\n", tx->tx_id);
                break;

            case OP_WITHDRAW:
                printf("[T%d] WITHDRAW account %d amount %d centavos\n", tx->tx_id, op->account_id, op->amount_centavos);
                if (!withdraw(op->account_id, op->amount_centavos)) {
                     printf("[T%d] WITHDRAW FAILED – insufficient funds. Transaction ABORTED.\n", tx->tx_id);
                     tx->status = TX_ABORTED;

                     pthread_mutex_lock(&tick_lock);
                     tx->actual_end = global_tick;
                     pthread_mutex_unlock(&tick_lock);

                     return NULL;
                }
                printf("[T%d] WITHDRAW successful\n", tx->tx_id);
                break;
            
            case OP_TRANSFER:
                printf("[T%d] TRANSFER from account %d to account %d amount %d centavos\n", tx->tx_id, op->account_id, op->target_account, op->amount_centavos);
                if(!transfer(op->account_id, op->target_account, op->amount_centavos)) {
                    printf("[T%d] TRANSFER FAILED – insufficient funds.Transaction ABORTED.\n", tx->tx_id);
                    tx->status = TX_ABORTED;

                    pthread_mutex_lock(&tick_lock);
                    tx->actual_end = global_tick;
                    pthread_mutex_unlock(&tick_lock);

                    return NULL;
                }
                printf("[T%d] TRANSFER successful\n", tx->tx_id);
                break;
            
            case OP_BALANCE: {
                int balance = get_balance(op->account_id);
                printf("[T%d] BALANCE account %d = PHP %d.%02d\n", tx->tx_id, op->account_id, balance / 100, balance % 100);
                break;
            }
            default:
                 fprintf(stderr, "[T%d] Unknown operation type %d – skipping\n", tx->tx_id, op->type);
                 break;
        }

        pthread_mutex_lock(&tick_lock);
        tx->wait_ticks += (global_tick - tick_before);
        pthread_mutex_unlock(&tick_lock);
    }

    pthread_mutex_lock(&tick_lock);
    tx->actual_end = global_tick;
    pthread_mutex_unlock(&tick_lock);

    tx->status = TX_COMMITTED;
    printf("[T%d] COMMITTED at tick %d (waited %d ticks total)\n", tx->tx_id, tx->actual_end, tx->wait_ticks);
    return NULL;

}
