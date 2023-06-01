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
    int * data;
    //Pointer to the start of the data array, used for freeing the memory
    int * data_free;
    int * __restrict new_data;
    // Temporary data for swapping and moving
    int * __restrict tmp_data;
    int merge_start;
    int data_n;
    int tmp_data_n;
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

void print_array(thread_data_t **thread_data, int threads_n) {
    for (int tid = 0; tid < threads_n; tid++) {
        printf("Thread %d: ", tid);
        for (int i = 0; i < thread_data[tid]->data_n; i++) {
            printf("%d ", thread_data[tid]->data[i]);
        }
        printf("\n");
    }
}

// Move values given by arrays of indices.
void move_values(thread_data_t *__restrict thread_0,
                 thread_data_t *__restrict thread_1) {
    int tmp_data_index = 0;
    int data_index = 0;
    int new_data_n = thread_1->tmp_data_n + thread_0->data_n-1;
    thread_0->new_data = (int *)malloc(sizeof(int) * (new_data_n));
    for(int i = 0; i < new_data_n; i++) {
        if(tmp_data_index < thread_1->tmp_data_n && thread_1->tmp_data[tmp_data_index] <= thread_0->data[data_index]) {
            thread_0->new_data[i] = thread_1->tmp_data[tmp_data_index];
            tmp_data_index++;
        }
        else {
            thread_0->new_data[i] = thread_0->data[data_index];
            data_index++;
        }
    }
    free(thread_0->data_free);
    thread_0->data = thread_0->new_data;
    thread_0->data_free = thread_0->new_data;
    thread_0->data_n = new_data_n;
    thread_1->tmp_data_n = 0;
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
    #pragma omp barrier
    // Find average of medians of the threads belonging to the group
    //This is calculated by all threads in the group which is unnecessary, but faster than 
    //the overhead of communicating the median_avg to all threads
    for (int tid = 0; tid < group_size; tid++) {
        median_avg += median(thread_data[tid+group_start]);
    }
    median_avg /= group_size;

    // Loop through pairs of threads
    const int tid0 = current_thread_id;
    const int tid1 = group_start + (group_size - 1) - thread_id_in_group;
    thread_data_t *thread0 = thread_data[tid0];
    #pragma omp barrier
    thread0->tmp_data_n = 0;
    int move_from_current_n = 0;
    if (thread_id_in_group < group_size / 2) {
        for (int i = 0; i < thread0->data_n; ++i) {
            if (thread0->data[i] >= median_avg) {
                thread0->tmp_data[move_from_current_n] = thread0->data[i];
                move_from_current_n++;
            }
        }
        thread0->tmp_data_n = move_from_current_n;
        thread0->data_n -= move_from_current_n;
    } else {
        // Walk forward through the array and find values that are smaller than
        // the median
        for (int i = 0; i < thread0->data_n; i++) {
            if (thread0->data[i] > median_avg) {
                break;
            }
            thread0->tmp_data[move_from_current_n] = thread0->data[i];
            move_from_current_n++;
        }
        thread0->tmp_data_n = move_from_current_n;
        thread0->data = thread0->data + move_from_current_n;
        thread0->data_n -= move_from_current_n;
    }
    thread_data_t *thread1 = thread_data[tid1];
    #pragma omp barrier
    move_values(thread0, thread1);

    #pragma omp barrier
    global_sort(thread_data,
                group_size / 2, current_thread_id);
}

int quick_sort_parallel(thread_data_t **thread_data, int threads_n) {
    #pragma omp parallel num_threads(threads_n)
    {
        int tid = omp_get_thread_num();
        qsort(thread_data[tid]->data, thread_data[tid]->data_n, sizeof(int),
              cmpfunc);
        global_sort(thread_data, threads_n, tid);
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
                return false;
            }
        }
    }
    return true;
}

void setup_threads() {}

int main() {

    // Actual system threads to use
    int num_omp_threads = 4;
    int n;
    //"Threads" to use for sorting (aka processors)
    int num_threads = num_omp_threads;
    // Random data
    // printf("Enter the number of data to be sorted (in millions): ");
    // scanf("%d", &n);
    // printf("Enter the number of threads: ");
    // scanf("%d", &threads);
    n = 40000;
    n *= 1000;
    int data_n_per_thread = n / num_threads;
#if USE_BUILTIN_QSORT
    printf("Using builtin qsort\n");
    int *data = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        data[i] = rand() % 90000 + 100;
    }
    qsort(data, n, sizeof(int), cmpfunc);
    return 0;
#endif
    // Allocate a bit more memory than needed to avoid reallocating
    const double alloc_multiplier = 1.1;
    const double alloc_multiplier_tmp = 0.6;

    thread_data_t **thread_data =
        (thread_data_t **)malloc(num_threads * sizeof(thread_data_t *));
    for (int tid = 0; tid < num_threads; tid++) {
        thread_data[tid] = (thread_data_t *)malloc(sizeof(thread_data_t));
        thread_data[tid]->tid = tid;
        thread_data[tid]->data =
            (int *)malloc(data_n_per_thread * alloc_multiplier * sizeof(int));
        thread_data[tid]->data_free = thread_data[tid]->data;
        thread_data[tid]->tmp_data = (int *)malloc(
            data_n_per_thread * alloc_multiplier_tmp * sizeof(int));
        thread_data[tid]->data_n = data_n_per_thread;
        for (int i = 0; i < data_n_per_thread; i++) {
            thread_data[tid]->data[i] = rand() % 90000 + 100;
        }
    }
    // Set omp threads
    omp_set_num_threads(num_omp_threads);
    //print_array(thread_data, num_threads);
    quick_sort_parallel(thread_data, num_threads);
    //print_array(thread_data, num_threads);
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
