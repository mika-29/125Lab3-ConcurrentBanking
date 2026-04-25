CC = gcc
CFLAGS = -Wall -Wextra -pthread -Iinclude
SRC = src/main.c src/bank.c src/transaction.c src/timer.c src/lock_mgr.c src/buffer_pool.c src/metrics.c src/utils.c
TARGET = bankdb
 
all:
	$(CC) $(CFLAGS) -O2 -o $(TARGET) $(SRC) -pthread
 
debug:
	$(CC) $(CFLAGS) -g -fsanitize=thread -o $(TARGET)_debug $(SRC) -pthread
 
test: all
	@echo ""
	@echo " Test 1: No Conflicts (trace_simple.txt)"
	@echo "========================================================" 
	./$(TARGET) --accounts=tests/accounts.txt --trace=tests/trace_simple.txt --deadlock=prevention --tick-ms=100
	@echo ""
	@echo " Test 2: Concurrent Readers (trace_readers.txt)"
	@echo "========================================================"
	./$(TARGET) --accounts=tests/accounts.txt --trace=tests/trace_readers.txt --deadlock=prevention --tick-ms=100
	@echo ""
	@echo " Test 3: Deadlock Prevention (trace_deadlock.txt)"
	@echo "========================================================"
	./$(TARGET) --accounts=tests/accounts.txt --trace=tests/trace_deadlock.txt --deadlock=prevention --tick-ms=100 --verbose
	@echo ""
	@echo " Test 4: Insufficient Funds (trace_abort.txt)"
	@echo "========================================================"
	./$(TARGET) --accounts=tests/accounts.txt --trace=tests/trace_abort.txt --deadlock=prevention --tick-ms=100
	@echo ""
	@echo " Test 5: Buffer Pool Saturation (trace_buffer.txt)"
	@echo "========================================================"
	./$(TARGET) --accounts=tests/accounts.txt --trace=tests/trace_buffer.txt --deadlock=prevention --tick-ms=100
 
clean:
	rm -f $(TARGET) $(TARGET)_debug

.PHONY: all debug test clean 
 