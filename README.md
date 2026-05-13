# Concurrent Banking - CMSC 125 [Lab3] 

## Group Members 
- Michaela F. Borces
- Marinelle Joan U. Tambolero

## Compilation

**Normal build:**
```bash
make
```

**Debug build (ThreadSanitizer):**
```bash
make debug
```

**Note for WSL2 users:** Run this before using the debug build:
```bash
sudo sysctl vm.mmap_rnd_bits=28
```

**Clean:**
```bash
make clean
```

## Usage

```bash
./bankdb --accounts=FILE --trace=FILE [OPTIONS]
```

**Required:**
- `--accounts=FILE` — Initial account balances file
- `--trace=FILE` — Transaction trace file

**Optional:**
- `--deadlock=prevention|detection` — Deadlock strategy (default: prevention)
- `--tick-ms=N` — Milliseconds per tick (default: 100)
- `--verbose` — Print detailed lock ordering logs

**Example:**
```bash
./bankdb --accounts=tests/accounts.txt --trace=tests/trace_deadlock.txt --deadlock=prevention --tick-ms=100 --verbose
```

**Run all tests:**
```bash
make test
```

## Implemented Features

- Multi-threaded transaction execution — each transaction runs in its own pthread
- Global simulation clock via dedicated timer thread with per-operation tick scheduling
- Reader-writer locks for concurrent account access (pthread_rwlock_t)
- Deadlock prevention via lock ordering — always locks lower account index first, breaking circular wait
- Bounded buffer pool (5 slots) using POSIX semaphores with producer-consumer pattern
- Four operation types: DEPOSIT, WITHDRAW, TRANSFER, BALANCE
- Clean abort handling for insufficient funds with proper buffer slot cleanup
- Transaction Performance Metrics — ActualStart, End, WaitTicks, and Status per transaction
- Money conservation check for transfer-only workloads
- Buffer Pool Report — total loads/unloads, peak usage, blocked operations

## Known Limitations

- ThreadSanitizer on WSL2 requires `sudo sysctl vm.mmap_rnd_bits=28` before running the debug build
