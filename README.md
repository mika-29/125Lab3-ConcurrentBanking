# Design Notes 

## Problem Analysis 
The Concurrent Banking System highlights the challenge of balancing performance and correctness in a multi-threaded environment. While multiple transactions should run simultaneously for efficiency, improper handling of shared data (e.g., account balances) can lead to race conditions and incorrect results.

The system must also handle deadlocks, especially in transactions involving multiple accounts, where threads may wait indefinitely due to circular dependencies. In addition, limited resources such as the buffer pool create contention, requiring proper management to ensure fair access and avoid delays.

Another challenge is enforcing deterministic execution despite the non-deterministic nature of thread scheduling. Transactions must follow a defined timeline using a global clock mechanism.

Overall, the problem involves coordinating transaction timing, managing limited resources, and ensuring safe access to shared data without sacrificing system efficiency.

## Solution Architecture
The banking system follows a **Multi-threaded Transaction Processing** design.

**>>Initialization**
The system loads initial account balances from `accounts.txt` and transaction
workloads from `trace.txt`. It initializes all locks, semaphores, and the 
buffer pool, then spawns the timer thread before launching transaction threads.

**>>Time Simulation**
A dedicated timer thread increments the global clock every N milliseconds and
broadcasts to all waiting transaction threads via `pthread_cond_broadcast()`.
Transactions wait for their scheduled `start_tick` using `wait_until_tick()`
before beginning execution.

**>>Transaction Execution**
Each transaction runs in its own pthread following this pipeline:

**Wait:** The transaction sleeps until the global clock reaches its `start_tick`.

**Load:** The transaction acquires a buffer slot from the pool using semaphores.
If the pool is full, it blocks until a slot is freed.

**Execute:** Each operation is processed in order:
   >DEPOSIT and WITHDRAW acquire a write lock on the account using
   `pthread_rwlock_wrlock()`.

   >BALANCE acquires a read lock using `pthread_rwlock_rdlock()`, allowing
   multiple transactions to read simultaneously.

   >TRANSFER locks both accounts in ascending ID order via `lock_mgr.c`
   to prevent deadlock through lock ordering.

**Unload:** The transaction releases its buffer slot and signals waiting threads.

**>>Termination**
Once all transactions finish, the system prints a full metrics report including
per-transaction stats, buffer pool usage, and a balance conservation check to
verify no money was created or lost during execution.