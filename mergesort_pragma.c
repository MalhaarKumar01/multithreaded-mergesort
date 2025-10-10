// mergesort_pragma.c
// gcc -O3 -fopenmp mergesort_pragma.c -o mergesort_pragma
// ./mergesort_pragma <num_elements> <num_threads>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#ifndef INSERTION_CUTOFF
#define INSERTION_CUTOFF 64
#endif

static inline double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

// ---------- helpers ----------
static void insertion_sort(int *a, int l, int r) {
    for (int i = l + 1; i <= r; i++) {
        int key = a[i], j = i - 1;
        while (j >= l && a[j] > key) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

static void merge(int *a, int *tmp, int l, int m, int r) {
    int i = l, j = m + 1, k = l;
    while (i <= m && j <= r)
        tmp[k++] = (a[i] <= a[j] ? a[i++] : a[j++]);
    while (i <= m) tmp[k++] = a[i++];
    while (j <= r) tmp[k++] = a[j++];
    memcpy(a + l, tmp + l, (r - l + 1) * sizeof(int));
}

// ---------- recursive mergesort ----------
static void mergesort_rec(int *a, int *tmp, int l, int r, int depth) {
    if (r - l + 1 <= INSERTION_CUTOFF) {
        insertion_sort(a, l, r);
        return;
    }

    int m = l + (r - l) / 2;

    // Spawn parallel tasks only beyond some recursion depth to avoid oversubscription
    if (depth < 4) {
        #pragma omp task shared(a, tmp)
        mergesort_rec(a, tmp, l, m, depth + 1);

        #pragma omp task shared(a, tmp)
        mergesort_rec(a, tmp, m + 1, r, depth + 1);

        #pragma omp taskwait
    } else {
        mergesort_rec(a, tmp, l, m, depth + 1);
        mergesort_rec(a, tmp, m + 1, r, depth + 1);
    }

    merge(a, tmp, l, m, r);
}

// ---------- check sorted ----------
static int is_sorted(int *a, size_t n) {
    for (size_t i = 1; i < n; i++)
        if (a[i - 1] > a[i]) return 0;
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <num_elements> <num_threads>\n", argv[0]);
        return 1;
    }

    size_t N = atoll(argv[1]);
    int T = atoi(argv[2]);

    int *a = malloc(N * sizeof(int));
    int *tmp = malloc(N * sizeof(int));
    if (!a || !tmp) {
        fprintf(stderr, "malloc fail\n");
        return 1;
    }

    // Fill randomly (can parallelize easily)
    double t0 = now_ms();
    #pragma omp parallel for num_threads(T)
    for (size_t i = 0; i < N; i++)
        a[i] = rand() & 0x7fffffff;
    double t1 = now_ms();

    omp_set_num_threads(T);

    double t2 = now_ms();
    #pragma omp parallel
    {
        #pragma omp single nowait
        mergesort_rec(a, tmp, 0, (int)N - 1, 0);
    }
    double t3 = now_ms();

    printf("Fill:   %.3f ms\nSort:   %.3f ms\nTotal:  %.3f ms  |  %s\n",
           t1 - t0, t3 - t2, t3 - t0, is_sorted(a, N) ? "OK" : "FAIL");

    free(a);
    free(tmp);
    return 0;
}

