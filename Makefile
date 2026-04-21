CC = gcc
CFLAGS = -Wall -Wextra -pthread -Iinclude
SRC = src/main.c src/bank.c src/transaction.c src/timer.c src/lock_mgr.c src/buffer_pool.c src/metrics.c src/utils.c
TARGET = bankdb
 
all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
 
debug:
	$(CC) $(CFLAGS) -g -fsanitize=thread -o $(TARGET)_debug $(SRC)
 
test: all
	./$(TARGET) --accounts=tests/accounts.txt --trace=tests/trace_simple.txt --deadlock=prevention --tick-ms=100
	./$(TARGET) --accounts=tests/accounts.txt --trace=tests/trace_readers.txt --deadlock=prevention --tick-ms=100
	./$(TARGET) --accounts=tests/accounts.txt --trace=tests/trace_deadlock.txt --deadlock=prevention --tick-ms=100 --verbose
	./$(TARGET) --accounts=tests/accounts.txt --trace=tests/trace_abort.txt --deadlock=prevention --tick-ms=100
	./$(TARGET) --accounts=tests/accounts.txt --trace=tests/trace_buffer.txt --deadlock=prevention --tick-ms=100
 
clean:
	rm -f $(TARGET) $(TARGET)_debug
 