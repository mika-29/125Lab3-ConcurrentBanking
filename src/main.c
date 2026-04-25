#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "bank.h"
#include "transaction.h"
#include "timer.h"
#include "lock_mgr.h"
#include "buffer_pool.h"
#include "metrics.h"
#include "utils.h"


//CLI option storage
static const char *accounts_file  = NULL;
static const char *trace_file     = NULL;
static const char *deadlock_strat = "prevention";   /* default */
static int         verbose        = 0;


//Print usage and exit.
static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s --accounts=FILE --trace=FILE\n"
        "          [--deadlock=prevention|detection]\n"
        "          [--tick-ms=N]  (default: 100)\n"
        "          [--verbose]\n",
        prog);
    exit(EXIT_FAILURE);
}

//Parse --key=value  or  --flag  arguments.
static void parse_args(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        if (strncmp(arg, "--accounts=", 11) == 0) {
            accounts_file = arg + 11;

        } else if (strncmp(arg, "--trace=", 8) == 0) {
            trace_file = arg + 8;

        } else if (strncmp(arg, "--deadlock=", 11) == 0) {
            deadlock_strat = arg + 11;

        } else if (strncmp(arg, "--tick-ms=", 10) == 0) {
            tick_interval_ms = atoi(arg + 10);
            if (tick_interval_ms <= 0) {
                fprintf(stderr, "ERROR: --tick-ms must be > 0\n");
                exit(EXIT_FAILURE);
            }

        } else if (strcmp(arg, "--verbose") == 0) {
            verbose = 1;

        } else {
            fprintf(stderr, "ERROR: unknown argument '%s'\n", arg);
            usage(argv[0]);
        }
    }

    if (!accounts_file || !trace_file) {
        fprintf(stderr, "ERROR: --accounts and --trace are required\n");
        usage(argv[0]);
    }

    if (strcmp(deadlock_strat, "prevention") != 0 &&
        strcmp(deadlock_strat, "detection")  != 0) {
        fprintf(stderr, "ERROR: --deadlock must be 'prevention' or 'detection'\n");
        usage(argv[0]);
    }
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);

    printf("=== Banking System Execution Log ===\n");
    printf("Tick interval : %d ms\n", tick_interval_ms);
    printf("Deadlock strategy : %s\n", deadlock_strat);
    if (verbose) printf("Verbose mode : ON\n");
    printf("\n");

    //initialize modules 
    bank_init();
    buffer_pool_init();
    timer_init();

    lock_mgr_describe();   

    // load accounts
    printf("\n[INIT] Loading accounts from '%s'...\n", accounts_file);
    if (parse_accounts(accounts_file) != 0) {
        fprintf(stderr, "FATAL: could not load accounts\n");
        return EXIT_FAILURE;
    }

    int initial_total = bank_total_centavos();
    printf("\nInitial total : PHP %d.%02d\n",
           initial_total / 100, initial_total % 100);
    bank_print_balances();

    printf("\n[INIT] Loading trace from '%s'...\n", trace_file);
    int num_tx = 0;
    Transaction *txs = parse_trace(trace_file, &num_tx);
    if (!txs) {
        fprintf(stderr, "FATAL: could not load trace\n");
        return EXIT_FAILURE;
    }
    printf("[INIT] %d transaction(s) loaded\n\n", num_tx);

    pthread_t timer_tid;
    if (pthread_create(&timer_tid, NULL, timer_thread, NULL) != 0) {
        perror("pthread_create timer");
        free(txs);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < num_tx; i++) {
        Transaction *tx = &txs[i];
        for (int j = 0; j < tx->num_ops; j++) {
            Operation *op = &tx->ops[j];

            if (verbose && op->type == OP_TRANSFER) {
                lock_mgr_log_order(op->account_id, op->target_account);
            }

            buffer_pool_load(op->account_id);
            if (op->type == OP_TRANSFER) {
                buffer_pool_load(op->target_account);
            }
        }

        if (pthread_create(&tx->thread, NULL, exec_transac, tx) != 0) {
            perror("pthread_create transaction");
            tx->status = TX_ABORTED;
        }
    }

    //wait all threads to finish 
    for (int i = 0; i < num_tx; i++) {
        if (txs[i].status != TX_ABORTED || txs[i].thread != 0) {
            pthread_join(txs[i].thread, NULL);
        }

        Transaction *tx = &txs[i];
        for (int j = 0; j < tx->num_ops; j++) {
            Operation *op = &tx->ops[j];
            buffer_pool_unload(op->account_id);
            if (op->type == OP_TRANSFER) {
                buffer_pool_unload(op->target_account);
            }
        }
    }

    // Stop timer thread
    pthread_mutex_lock(&tick_lock);
    simul_running = 0;
    pthread_cond_broadcast(&tick_changed);
    pthread_mutex_unlock(&tick_lock);
    pthread_cancel(timer_tid); 
    pthread_join(timer_tid, NULL);

    //printing
    printf("\n=== Summary ===\n");

    int final_total = bank_total_centavos();
    printf("Final total   : PHP %d.%02d\n",
           final_total / 100, final_total % 100);
    bank_print_balances();

    if (final_total == initial_total) {
        printf("\nConservation check : PASSED (no money created or destroyed)\n");
    } else {
        printf("\nConservation check : FAILED "
               "(initial=%d centavos, final=%d centavos, delta=%d)\n",
               initial_total, final_total, final_total - initial_total);
    }

    print_metrics(txs, num_tx);
    buffer_pool_print_stats();

    timer_cleanup();
    buffer_pool_destroy();
    bank_destroy();
    free(txs);

    return EXIT_SUCCESS;
}