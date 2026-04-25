#include <stdio.h>
#include "metrics.h"
#include "transaction.h"

void print_metrics(Transaction *transactions, int num_transactions) {
    printf("\n=== Transaction Performance Metrics ===\n");
    printf("%-6s | %-10s | %-12s | %-5s | %-10s | %-10s\n",
           "TxID", "StartTick", "ActualStart", "End", "WaitTicks", "Status");
    printf("-------|------------|--------------|-------|------------|----------\n");

    int total_wait = 0;
    int committed = 0;
    int aborted = 0;
    int max_end_tick = 0;

    for (int i = 0; i < num_transactions; i++) {
        Transaction *tx = &transactions[i];

        const char *status_str;
        switch (tx->status) {
            case TX_RUNNING: 
                status_str = "RUNNING"; 
                break;

            case TX_COMMITTED: 
                status_str = "COMMITTED"; 
                committed++; 
                break;

            case TX_ABORTED: 
                status_str = "ABORTED"; 
                aborted++; 
                break;

            default: 
            status_str = "PENDING"; 
            break;
        }

        printf("T%-5d | %-10d | %-12d | %-5d | %-10d | %s\n",
                tx->tx_id, tx->start_tick,
                tx->actual_start, tx->actual_end,
                tx->wait_ticks, status_str);
    
        total_wait += tx->wait_ticks;
    }

    if (tx->actual_end > max_end_tick) {
        max_end_tick = tx->actual_end;
    }

    double avg_wait   = num_transactions > 0 ? (double)total_wait / num_transactions : 0.0;
    double throughput = max_end_tick > 0
                        ? (double)num_transactions / max_end_tick
                        : 0.0;
 
    printf("\nTotal transactions : %d\n", num_transactions);
    printf("Committed          : %d\n", committed);
    printf("Aborted            : %d\n", aborted);
    printf("Total ticks        : %d\n", max_end_tick);
    printf("Average wait time  : %.2f ticks\n", avg_wait);
    printf("Throughput         : %.2f tx/tick\n", throughput);
}