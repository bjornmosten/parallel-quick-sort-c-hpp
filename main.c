#include <omp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define USE_BUILTIN_QSORT 0

typedef struct thread_data {
    int *data;
    int data_n;
    int low_last;
} thread_data_t;

void swap_values(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}
int cmpfunc(const void *a, const void *b) {
    //
    return (*(int *)a - *(int *)b);
}

int partition(int *data, int data_n) {
    int pivot = data_n / 2;
    int pivot_value = data[pivot];
    swap_values(data + pivot, data + data_n - 1);
    int store_index = 0;
    for (int i = 0; i < data_n - 1; i++) {
        if (data[i] < pivot_value) {
            swap_values(data + i, data + store_index);
            store_index++;
        }
    }
    swap_values(data + store_index, data + data_n - 1);
    return store_index;
}

int quick_sort_local(int *data, int data_n) {
    if (data_n <= 1) {
        return data[0];
    }
    int pivot = partition(data, data_n);
    quick_sort_local(data, pivot);
    quick_sort_local(data + pivot + 1, data_n - pivot - 1);
    return data[pivot];
}

int median(thread_data_t *thread_data) {
    int *data = thread_data->data;
    int data_n = thread_data->data_n;
    quick_sort_local(data, data_n);
    return data[data_n / 2];
}

void print_array(thread_data_t **thread_data, int threads_n) {
    for (int tid = 0; tid < threads_n; tid++) {
        printf("Thread %d: ", tid);
        for (int i = 0; i < thread_data[tid]->data_n; i++) {
            printf("%d ", thread_data[tid]->data[i]);
        }
        printf("\n");
    }
}

void move_values(thread_data_t *__restrict thread_0,
                 thread_data_t *__restrict thread_1, int *move_from_0,
                 int *move_from_1, int move_from_0_n, int move_from_1_n) {
    for (int hi = 0; hi < move_from_1_n; hi++) {
        int index = move_from_1[hi];
        int value = thread_1->data[index];
        thread_1->data[index] = thread_0->data[move_from_0[hi]];
        thread_0->data[move_from_0[hi]] = value;
    }
    for (int li = 0; li + move_from_1_n < move_from_0_n; li++) {
        // Start from the values that could not be swapped and have to be added
        int move_index = move_from_0[li + (move_from_1_n)];
        thread_1->data[thread_1->data_n] = thread_0->data[move_index];
        thread_0->data[move_index] = thread_0->data[thread_0->data_n - 1];
        thread_0->data_n--;
        thread_1->data_n++;
    }
}

void global_sort(thread_data_t ** all_threads, int group_size, int thread_id) {
    if (group_size <= 1) {
        return;
    }
    int median_avg = 0;
    int partner_thread_id = group_size - thread_id - 1;
    thread_data_t * current_thread = all_threads[thread_id];
    thread_data_t * partner_thread = all_threads[partner_thread_id];
    int current_thread_median = median(current_thread); 


// Exchange data
    int tid0 = (g) + group_thread_start;
    int tid1 = group_thread_end - (g)-1;
    thread0 = thread_data[tid0];
    thread1 = thread_data[tid1];
    // Values to move from low (0) to high (1)
    int move_from_n = 0;
    // If more values are going to be moved from low to high, swap
    // values and then perform transfers which increase or decrease
    qsort(thread1->data, thread1->data_n, sizeof(int), cmpfunc);
}

int quick_sort_parallel(thread_data_t **thread_data, int threads_n) {

    for (int tid = 0; tid < threads_n; ++tid) {
#pragma omp task
        qsort(thread_data[tid]->data, thread_data[tid]->data_n, sizeof(int),
              cmpfunc);
    }
#pragma omp taskwait
    global_sort(thread_data, threads_n, 0);
    printf("Global sort done\n");
    return 0;
}

bool validate(thread_data_t **thread_data, int threads_n) {
    for (int tid = 0; tid < threads_n; tid++) {
        if (tid > 0) {
            if (thread_data[tid - 1]->data[thread_data[tid - 1]->data_n - 1] >
                thread_data[tid]->data[0]) {
                printf("Error: %d > %d\n",
                       thread_data[tid - 1]->data[thread_data[tid]->data_n - 1],
                       thread_data[tid]->data[0]);
                return false;
            }
        }
        for (int i = 0; i < thread_data[tid]->data_n - 1; i++) {
            if (thread_data[tid]->data[i] > thread_data[tid]->data[i + 1]) {
                return false;
            }
        }
    }
    return true;
}

bool validate_builtin(int *data, int n) {
    for (int i = 0; i < n - 1; i++) {
        if (data[i] > data[i + 1]) {
            return false;
        }
    }
    return true;
}

void setup_threads() {}

int main() {

    int n, num_procs;
    int num_threads = 16;
    // Random data
    // printf("Enter the number of data to be sorted (in millions): ");
    // scanf("%d", &n);
    // printf("Enter the number of threads: ");
    // scanf("%d", &threads);
    n = 200000;
    num_procs = 32;
    n *= 1000;
    int randmax = (int)1E8;
    int data_n_per_thread = n / num_procs;

    omp_set_dynamic(0);
    omp_set_nested(1);
    omp_set_num_threads(num_threads);
    srand(52151);

#if USE_BUILTIN_QSORT
    int *data = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        data[i] = rand() % randmax;
    }
    printf("Using builtin qsort\n");
    qsort(data, n, sizeof(int), cmpfunc);
    if (validate_builtin(data, n)) {
        printf("Success\n");
    } else {
        printf("Failure\n");
    }
    return 0;
#endif

    thread_data_t **thread_data =
        (thread_data_t **)malloc(num_procs * sizeof(thread_data_t *));
    void *thread_data_mem = malloc(num_procs * sizeof(thread_data_t));
    for (int tid = 0; tid < num_procs; tid++) {
        thread_data[tid] = (thread_data_t *)thread_data_mem + tid;
        thread_data[tid]->data =
            (int *)malloc(data_n_per_thread * 1.1 * sizeof(int));
        thread_data[tid]->data_n = data_n_per_thread;
        for (int i = 0; i < data_n_per_thread; i++) {
            thread_data[tid]->data[i] = rand() % randmax;
        }
    }

#pragma omp parallel num_threads(num_threads)
    {
#pragma omp single nowait
        quick_sort_parallel(thread_data, num_procs);
    }
    printf("\n");
    bool valid = validate(thread_data, num_procs);
    if (valid) {
        printf("Valid\n");
    } else {
        printf("Invalid\n");
    }

    printf("\n");
    return 0;
}
