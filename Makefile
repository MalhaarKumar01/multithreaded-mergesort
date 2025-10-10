# =============================================
#  MULTITHREADED MERGESORT — Cross-Platform Makefile
#  Supports macOS (Clang+libomp), Linux (GCC), Windows (MinGW/MSYS/WSL)
# =============================================

# ---------- Compiler & Base Flags ----------
CC       := $(shell which clang 2>/dev/null || which gcc 2>/dev/null || echo clang)
CFLAGS   := -O3 -Wall -Wextra -std=c11 -pthread -funroll-loops
UNAME_S  := $(shell uname -s 2>/dev/null || echo Windows)

# ---------- Default Threads ----------
THREADS  := 8

# ---------- CPU optimization ----------
ifeq ($(UNAME_S),Darwin)
    CFLAGS += -mcpu=apple-m3
endif

# ---------- Executable names ----------
THREAD_EXE := mergesort_threaded
UNROLL_EXE := mergesort_unrolling
PRAGMA_EXE := mergesort_pragma

# ---------- Sources ----------
THREAD_SRC := mergesort_threaded.c
UNROLL_SRC := mergesort_with_unrolling.c
PRAGMA_SRC := mergesort_pragma.c

# ---------- Detect and Configure OpenMP ----------
ifeq ($(UNAME_S),Darwin)
    # macOS (Clang) setup
    LIBOMP_PREFIX := $(shell brew --prefix libomp 2>/dev/null)
    ifneq ($(LIBOMP_PREFIX),)
        OMP_CFLAGS  := -Xpreprocessor -fopenmp -I$(LIBOMP_PREFIX)/include
        OMP_LDFLAGS := -L$(LIBOMP_PREFIX)/lib -lomp
    else
        $(warning ⚠️ libomp not found. Run: brew install libomp)
        OMP_CFLAGS  :=
        OMP_LDFLAGS :=
    endif
else ifeq ($(UNAME_S),Linux)
    # Linux (GCC/Clang)
    OMP_CFLAGS  := -fopenmp
    OMP_LDFLAGS := -fopenmp
else
    # Windows (MSYS/MinGW/Cygwin)
    OMP_CFLAGS  := -fopenmp
    OMP_LDFLAGS := -fopenmp
endif

# ---------- Targets ----------
.PHONY: all clean run-thread run-unroll run-pragma

all: $(THREAD_EXE) $(UNROLL_EXE) $(PRAGMA_EXE)

$(THREAD_EXE): $(THREAD_SRC)
	@echo "🧵 Building $@ ..."
	$(CC) $(CFLAGS) $< -o $@

$(UNROLL_EXE): $(UNROLL_SRC)
	@echo "🔁 Building $@ ..."
	$(CC) $(CFLAGS) $< -o $@

$(PRAGMA_EXE): $(PRAGMA_SRC)
	@echo "⚙️  Building $@ with OpenMP ($(UNAME_S)) ..."
	$(CC) $(CFLAGS) $(OMP_CFLAGS) $< -o $@ $(OMP_LDFLAGS)

# ---------- Run Targets (Default threads = 8) ----------
run-thread: $(THREAD_EXE)
	@echo "▶️  Running $(THREAD_EXE) with $(THREADS) threads..."
	./$(THREAD_EXE) 100000000 $(THREADS)

run-unroll: $(UNROLL_EXE)
	@echo "▶️  Running $(UNROLL_EXE) with $(THREADS) threads..."
	./$(UNROLL_EXE) 100000000 $(THREADS)

run-pragma: $(PRAGMA_EXE)
	@echo "▶️  Running $(PRAGMA_EXE) with $(THREADS) threads..."
	./$(PRAGMA_EXE) 100000000 $(THREADS)

# ---------- Clean ----------
clean:
	rm -f $(THREAD_EXE) $(UNROLL_EXE) $(PRAGMA_EXE)

