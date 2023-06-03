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

// Use builtin qsort if 1, otherwise use custom parallel quicksort
#define USE_BUILTIN_QSORT 0
typedef struct thread_data {
    // Thread data
    int *data;
    // Pointer to the start of the data array, used for freeing the memory
    int *data_free;
    int *new_data;
    // Temporary data for swapping and moving
    int data_n;
    int new_data_n;
    int split_index;
    int tid;
} thread_data_t;

void swap_values(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}
// Compare function for qsort
int cmpfunc(const void *a, const void *b) {
    //
    return (*(int *)a - *(int *)b);
}

// Median of one thread
inline int median(thread_data_t *thread_data) {
    int *data = thread_data->data;
    int data_n = thread_data->data_n;
    return data[data_n / 2];
}

void print_one_thread(thread_data_t *thread_data) {
    printf("Thread %d data: ", thread_data->tid);
    for (int i = 0; i < thread_data->data_n; i++) {
        printf("%d ", thread_data->data[i]);
    }
    printf("\nThread %d merge: ", thread_data->tid);
    printf("\n");
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

int * merge_data(thread_data_t *__restrict thread_0,
                thread_data_t *__restrict thread_1, bool is_lower) {
    int thread_0_index;
    int thread_1_index;
    int * new_data;
    int new_data_n;
    int i = 0;
    if(is_lower) {
        thread_0_index = 0;
        thread_1_index = 0;
        new_data_n = thread_0->split_index + thread_1->split_index;
        new_data = (int *)malloc(sizeof(int) * new_data_n);
        while (thread_0_index < thread_0->split_index &&
               thread_1_index < thread_1->split_index) {
            if (thread_0->data[thread_0_index] <
                thread_1->data[thread_1_index]) {
                new_data[i] = thread_0->data[thread_0_index];
                thread_0_index++;
            } else {
                new_data[i] = thread_1->data[thread_1_index];
                thread_1_index++;
            }
            i++;
        }
        while (thread_0_index < thread_0->split_index && i < new_data_n) {
            new_data[i] = thread_0->data[thread_0_index];
            thread_0_index++;
            i++;
        }
        while (thread_1_index < thread_1->split_index && i < new_data_n) {
            new_data[i] = thread_1->data[thread_1_index];
            thread_1_index++;
            i++;
        }
    } else {
        thread_0_index = thread_0->split_index;
        thread_1_index = thread_1->split_index;
        new_data_n = thread_0->data_n - thread_0->split_index + thread_1->data_n - thread_1->split_index;
        new_data = (int *)malloc(sizeof(int) * new_data_n);
        while (thread_0_index < thread_0->data_n &&
               thread_1_index < thread_1->data_n) {
            if (thread_0->data[thread_0_index] <
                thread_1->data[thread_1_index]) {
                new_data[i] = thread_0->data[thread_0_index];
                thread_0_index++;
            } else {
                new_data[i] = thread_1->data[thread_1_index];
                thread_1_index++;
            }
            i++;
        }
        while (thread_0_index < thread_0->data_n && i < new_data_n) {
            new_data[i] = thread_0->data[thread_0_index];
            thread_0_index++;
            i++;
        }
        while (thread_1_index < thread_1->data_n && i < new_data_n) {
            new_data[i] = thread_1->data[thread_1_index];
            thread_1_index++;
            i++;
        }
    }
    thread_0->new_data_n = new_data_n;
    return new_data;
}

//find split index with binary search, return index
int find_split(int * data, int data_n, int median) {
    int start = 0;
    int end = data_n - 1;
    int mid = (start + end) / 2;
    while (start <= end) {
        mid = (start + end) / 2;
        if (data[mid] == median) {
            return mid;
        } else if (data[mid] < median) {
            start = mid + 1;
        } else {
            end = mid - 1;
        }
    }
    return start;
}

void global_sort(thread_data_t **thread_data, int group_size,
                 int current_thread_id) {
    if (group_size == 1) {
        return;
    }
    int group_id = current_thread_id / group_size;
    int group_start = group_id * group_size;
    int thread_id_in_group = current_thread_id % group_size;
    int median_avg = 0;
    // Find average of medians of the threads belonging to the group
    // This is calculated by all threads in the group which is unnecessary, but
    // faster than the overhead of communicating the median_avg to all threads
    for (int tid = 0; tid < group_size; tid++) {
        median_avg += median(thread_data[tid + group_start]);
    }
    median_avg /= group_size;

    printf("Thread %d: median_avg %d\n", current_thread_id, median_avg);
    // Loop through pairs of threads
    const int tid0 = current_thread_id;
    const int tid1 = group_start + (group_size - 1) - thread_id_in_group;
    thread_data_t *thread0 = thread_data[tid0];
    thread_data_t *thread1 = thread_data[tid1];
    int move_from_current_n = 0;
    int split_index = find_split(thread0->data, thread0->data_n, median_avg);
    thread0->split_index = split_index;
#pragma omp barrier
    int * new_data = merge_data(thread0, thread1, (tid0 < tid1));
#pragma omp barrier
    free(thread0->data);
    thread0->data = new_data;
    thread0->data_n = thread0->new_data_n;
#pragma omp barrier
    global_sort(thread_data, group_size / 2, current_thread_id);
}

int quick_sort_parallel(thread_data_t **thread_data, int threads_n) {
#pragma omp parallel for
    for (int i = 0; i < threads_n; i++) {
            qsort(thread_data[i]->data, thread_data[i]->data_n, sizeof(int),
                  cmpfunc);
    }

#pragma omp parallel
    {
        for (int i = 0; i < threads_n; i++) {
#pragma omp single nowait
            {
                int tid = omp_get_thread_num();
                global_sort(thread_data, threads_n, tid);
            }
        }
    }
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
                printf("Error: %d > %d\n", thread_data[tid]->data[i],
                       thread_data[tid]->data[i + 1]);
                return false;
            }
        }
    }
    return true;
}

void setup_threads() {}

int main() {

    // Actual system threads to use
    int num_omp_threads = 8;
    int n;
    //"Threads" to use for sorting (aka processors)
    int num_threads = num_omp_threads;
    // Random data
    // printf("Enter the number of data to be sorted (in millions): ");
    // scanf("%d", &n);
    // printf("Enter the number of threads: ");
    // scanf("%d", &threads);
    n = 4;
    n *= 10;
    int data_n_per_thread = n / num_threads;
    // Allocate a bit more memory than needed to avoid reallocating
    const double alloc_multiplier = 2;
    const double alloc_multiplier_tmp = 0.8;

    int *builtin_qsort_data = (int *)malloc(n * sizeof(int));
    thread_data_t **thread_data =
        (thread_data_t **)malloc(num_threads * sizeof(thread_data_t *));
    for (int tid = 0; tid < num_threads; tid++) {
        thread_data[tid] = (thread_data_t *)malloc(sizeof(thread_data_t));
        thread_data[tid]->tid = tid;
        thread_data[tid]->data =
            (int *)malloc(data_n_per_thread * alloc_multiplier * sizeof(int));
        thread_data[tid]->data_free = thread_data[tid]->data;
        thread_data[tid]->data_n = data_n_per_thread;
        for (int i = 0; i < data_n_per_thread; i++) {
            int rand_val = rand() % 90000 + 100;
            thread_data[tid]->data[i] = rand_val;
            builtin_qsort_data[tid * data_n_per_thread + i] = rand_val;
        }
    }
#if USE_BUILTIN_QSORT
    printf("Using builtin qsort\n");
    int *data = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        data[i] = rand() % 90000 + 100;
    }
    qsort(data, n, sizeof(int), cmpfunc);
    return 0;
#endif
    // Set omp threads
    omp_set_num_threads(num_omp_threads);
    quick_sort_parallel(thread_data, num_threads);
    print_array(thread_data, num_threads);
    // Calculate number of values in each thread
    int total_n = 0;
    for (int tid = 0; tid < num_threads; tid++) {
        printf("Thread %d: %d\n", tid, thread_data[tid]->data_n);
        total_n += thread_data[tid]->data_n;
    }
    printf("Total: %d\n", total_n);
    printf("\n");
    bool valid = validate(thread_data, num_threads);
    if (valid) {
        printf("Valid\n");
    } else {
        printf("Invalid\n");
    }

    printf("\n");

    return 0;
}
