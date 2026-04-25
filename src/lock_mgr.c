#include <stdio.h>
#include "lock_mgr.h"

void lock_mgr_describe(void) {       
    printf("[LOCK MGR] Strategy: DEADLOCK PREVENTION via lock ordering\n");
    printf("[LOCK MGR] Transfers always lock the lower account-index first.\n");
    printf("[LOCK MGR] Coffman condition broken: CIRCULAR WAIT\n");
}

void lock_mgr_log_order(int from_id, int to_id) {
    int from_idx = bank_find_account(from_id);
    int to_idx   = bank_find_account(to_id);

    if (from_idx == -1 || to_idx == -1) return;

    int first_id  = (from_idx < to_idx) ? from_id : to_id;
    int second_id = (from_idx < to_idx) ? to_id   : from_id;

    printf("[DEADLOCK PREVENTED] Lock ordering: acquiring account %d first, "
           "then account %d\n", first_id, second_id);
}