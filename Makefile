# Makefile for MULTITHREADED-MERGESORT

CC      := clang
CFLAGS  := -O3 -Wall -Wextra -std=c11 -pthread -mcpu=apple-m3 -funroll-loops

# Executable names
THREAD_EXE    := mergesort_threaded
UNROLL_EXE    := mergesort_unrolling

# Sources
THREAD_SRC    := mergesort_threaded.c
UNROLL_SRC    := mergesort_with_unrolling.c

.PHONY: all clean run-thread run-unroll

all: $(THREAD_EXE) $(UNROLL_EXE)

$(THREAD_EXE): $(THREAD_SRC)
	$(CC) $(CFLAGS) $< -o $@

$(UNROLL_EXE): $(UNROLL_SRC)
	$(CC) $(CFLAGS) $< -o $@

run-thread: $(THREAD_EXE)
	./$(THREAD_EXE) 100000000 12

run-unroll: $(UNROLL_EXE)
	./$(UNROLL_EXE) 100000000 12

clean:
	rm -f $(THREAD_EXE) $(UNROLL_EXE)
