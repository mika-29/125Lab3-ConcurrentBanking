#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "bank.h"
#include "transaction.h"

int parse_accounts(const char *filename) {           //Returns 0 on success, -1 on error. 
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("[UTILS] fopen accounts file");
        return -1;
    }

    char line[256];
    int  line_no = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_no++;

        char *p = line;
        while (isspace((unsigned char)*p)) p++;

        if (*p == '\0' || *p == '#') continue;

        int account_id, balance;
        if (sscanf(p, "%d %d", &account_id, &balance) != 2) {
            fprintf(stderr, "[UTILS] accounts.txt line %d: bad format\n",
                    line_no);
            fclose(fp);
            return -1;
        }

        if (bank_add_account(account_id, balance) != 0) {
            fprintf(stderr, "[UTILS] Failed to add account %d\n", account_id);
            fclose(fp);
            return -1;
        }

        printf("[INIT] Account %d loaded with PHP %d.%02d\n",
               account_id, balance / 100, balance % 100);
    }

    fclose(fp);
    return 0;
}

Transaction *parse_trace(const char *filename, int *out_count) {                 //Multiple lines with the same T<id> and start_tick are grouped into one
    FILE *fp = fopen(filename, "r");                                             //Transaction struct (each line becomes one Operation).
    if (!fp) {
        perror("[UTILS] fopen trace file");
        return NULL;
    }

    char line[512];                                                               //First pass: count distinct transaction IDs. 
    int  max_tx_id  = -1;
    int  line_no    = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_no++;
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0' || *p == '#') continue;

        int tx_id, start_tick;
        char op_str[32];
        if (sscanf(p, "T%d %d %31s", &tx_id, &start_tick, op_str) < 3) {
            fprintf(stderr, "[UTILS] trace line %d: bad format\n", line_no);
            fclose(fp);
            return NULL;
        }
        if (tx_id > max_tx_id) max_tx_id = tx_id;
    }

    if (max_tx_id < 0) {
        fprintf(stderr, "[UTILS] trace file has no transactions\n");
        fclose(fp);
        return NULL;
    }

    int num_tx = max_tx_id + 1; 
    Transaction *txs = calloc(num_tx, sizeof(Transaction));
    if (!txs) {
        perror("[UTILS] calloc transactions");
        fclose(fp);
        return NULL;
    }

    for (int i = 0; i < num_tx; i++) {                                          //Initialise all slots (some IDs might be unused). 
        txs[i].tx_id      = -1;   
        txs[i].num_ops    = 0;
        txs[i].start_tick = 0;
        txs[i].status     = TX_RUNNING;
    }

    rewind(fp);                                                                   //Second pass: fill operations.
    line_no = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_no++;
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0' || *p == '#') continue;

        int  tx_id, start_tick;
        char op_str[32];

        if (sscanf(p, "T%d %d %31s", &tx_id, &start_tick, op_str) < 3) {
            fprintf(stderr, "[UTILS] trace line %d: bad format\n", line_no);
            free(txs);
            fclose(fp);
            return NULL;
        }

        Transaction *tx = &txs[tx_id];

        if (tx->tx_id == -1) {
            tx->tx_id      = tx_id;
            tx->start_tick = start_tick;
        }

        if (tx->num_ops >= 256) {
            fprintf(stderr, "[UTILS] T%d has more than 256 ops\n", tx_id);
            free(txs);
            fclose(fp);
            return NULL;
        }

        Operation *op = &tx->ops[tx->num_ops];

        if (strcmp(op_str, "DEPOSIT") == 0) {
            int acc, amount;
            if (sscanf(p, "T%*d %*d DEPOSIT %d %d", &acc, &amount) != 2) {
                fprintf(stderr, "[UTILS] trace line %d: bad DEPOSIT args\n",
                        line_no);
                free(txs);
                fclose(fp);
                return NULL;
            }
            op->type            = OP_DEPOSIT;
            op->account_id      = acc;
            op->amount_centavos = amount;
            op->target_account  = -1;

        } else if (strcmp(op_str, "WITHDRAW") == 0) {
            int acc, amount;
            if (sscanf(p, "T%*d %*d WITHDRAW %d %d", &acc, &amount) != 2) {
                fprintf(stderr, "[UTILS] trace line %d: bad WITHDRAW args\n",
                        line_no);
                free(txs);
                fclose(fp);
                return NULL;
            }
            op->type            = OP_WITHDRAW;
            op->account_id      = acc;
            op->amount_centavos = amount;
            op->target_account  = -1;

        } else if (strcmp(op_str, "TRANSFER") == 0) {
            int from_acc, to_acc, amount;
            if (sscanf(p, "T%*d %*d TRANSFER %d %d %d",
                       &from_acc, &to_acc, &amount) != 3) {
                fprintf(stderr, "[UTILS] trace line %d: bad TRANSFER args\n",
                        line_no);
                free(txs);
                fclose(fp);
                return NULL;
            }
            op->type            = OP_TRANSFER;
            op->account_id      = from_acc;
            op->target_account  = to_acc;
            op->amount_centavos = amount;

        } else if (strcmp(op_str, "BALANCE") == 0) {
            int acc;
            if (sscanf(p, "T%*d %*d BALANCE %d", &acc) != 1) {
                fprintf(stderr, "[UTILS] trace line %d: bad BALANCE args\n",
                        line_no);
                free(txs);
                fclose(fp);
                return NULL;
            }
            op->type            = OP_BALANCE;
            op->account_id      = acc;
            op->amount_centavos = 0;
            op->target_account  = -1;

        } else {
            fprintf(stderr, "[UTILS] trace line %d: unknown op '%s'\n",
                    line_no, op_str);
            free(txs);
            fclose(fp);
            return NULL;
        }

        tx->num_ops++;
    }

    fclose(fp);

    int actual = 0;
    for (int i = 0; i < num_tx; i++) {
        if (txs[i].tx_id != -1) {
            if (actual != i) txs[actual] = txs[i];
            actual++;
        }
    }

    *out_count = actual;
    return txs;
}