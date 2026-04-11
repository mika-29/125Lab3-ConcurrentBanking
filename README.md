# Design Notes 

## Problem Analysis 
The Concurrent Banking System highlights the challenge of balancing performance and correctness in a multi-threaded environment. While multiple transactions should run simultaneously for efficiency, improper handling of shared data (e.g., account balances) can lead to race conditions and incorrect results.

The system must also handle deadlocks, especially in transactions involving multiple accounts, where threads may wait indefinitely due to circular dependencies. In addition, limited resources such as the buffer pool create contention, requiring proper management to ensure fair access and avoid delays.

Another challenge is enforcing deterministic execution despite the non-deterministic nature of thread scheduling. Transactions must follow a defined timeline using a global clock mechanism.

Overall, the problem involves coordinating transaction timing, managing limited resources, and ensuring safe access to shared data without sacrificing system efficiency.
