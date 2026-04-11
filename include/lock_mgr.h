#define MAX_TRANSACTIONS 256

typedef struct {
    int tx_id;
    int waiting_for_tx;         // -1 if not waiting
    int waiting_for_account;
} WaitForEntry;

WaitForEntry wait_graph[MAX_TRANSACTIONS];
pthread_mutex_t graph_lock;