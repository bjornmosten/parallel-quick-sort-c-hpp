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


typedef struct thread_data {
    int * data;
    int data_n;
} thread_data_t;

void swap_values(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int partition(int * data, int data_n) {
    int pivot = data_n-1;
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

int quick_sort_local(int * data, int data_n) {
    if (data_n <= 1) {
        return data[0];
    }
    int pivot = partition(data, data_n);
    quick_sort_local(data, pivot);
    quick_sort_local(data + pivot + 1, data_n - pivot - 1);
    return data[pivot];
}

int median(thread_data_t * thread_data) {
    int * data = thread_data->data;
    int data_n = thread_data->data_n;
    quick_sort_local(data, data_n);
    return data[data_n / 2];
}

void print_array(thread_data_t ** thread_data, int threads_n) {
    for (int tid = 0; tid < threads_n; tid++) {
        printf("Thread %d: ", tid);
        for (int i = 0; i < thread_data[tid]->data_n; i++) {
            printf("%d ", thread_data[tid]->data[i]);
        }
        printf("\n");
    }

}


void global_sort(thread_data_t ** thread_data, int group_thread_start, int group_thread_end) {
    int group_size = group_thread_end - group_thread_start;
    if (group_size == 1) {
        return;
    }
    int median_avg = 0;
    for(int tid = group_thread_start; tid < group_thread_end; tid++) {
        median_avg += median(thread_data[tid]);
    }
    median_avg /= group_size;
    // Exchange data
    for (int g = 0; g < group_size / 2; g++) {
        const int tid0 = g + group_thread_start;
        const int tid1 = group_thread_end - g - 1;
        thread_data_t *thread0 = thread_data[tid0];
        thread_data_t *thread1 = thread_data[tid1];
        int values_low_to_high = 0;
        int values_high_to_low = 0;
        int move_from_low[thread_data[tid0]->data_n];
        int move_from_low_n = 0;
        for (int i = 0; i < thread0->data_n; i++) {
            if (thread0->data[i] > median_avg) {
                move_from_low[move_from_low_n] = i;
                move_from_low_n++;
            }
        }
        int move_from_high[thread_data[tid1]->data_n];
        int move_from_high_n = 0;
        for(int i = 0; i < thread1->data_n; i++) {
            if (thread1->data[i] <= median_avg) {
                move_from_high[move_from_high_n] = i;
                move_from_high_n++;
            }
        }
        //If more values are going to be moved from low to high, swap values and then
        //perform transfers which increase or decrease data_n
        if(move_from_low_n <= move_from_high_n) { 
            int li = 0;
            for (li = 0; li < move_from_low_n; li++) {
                int index = move_from_low[li];
                int value = thread0->data[index];
                thread0->data[index] = thread1->data[move_from_high[li]];
                thread1->data[move_from_high[li]] = value;
            }
            for(int hi = 0; hi+li < move_from_high_n; hi++) {
                int move_index = move_from_high[li + hi];
                thread0->data[thread0->data_n] = thread1->data[move_index];

                int last_non_zero = 0;
                for(int i = thread_data[tid1]->data_n-1; i >= 0; i--) {
                    if(thread1->data[i] != 0) {
                        last_non_zero = thread1->data[i];
                        break;
                    }
                }
                thread1->data[move_index] = last_non_zero;

                thread0->data_n++;
                thread1->data_n--;
            }
        }
        //If more values are going to be moved from high to low, swap values and then
        //perform transfers which increase or decrease data_n
        if(move_from_low_n > move_from_high_n) {
            int hi = 0;
            for (hi = 0; hi < move_from_high_n; hi++) {
                int index = move_from_high[hi];
                int value = thread1->data[index];
                thread1->data[index] = thread0->data[move_from_low[hi]];
                thread0->data[move_from_low[hi]] = value;
            }
            for(int li = 0; li+hi < move_from_low_n; li++) {
                int move_index = move_from_low[li + hi];
                thread1->data[thread1->data_n] = thread0->data[move_index];
                //Find last non-zero element
                int last_non_zero = 0;
                for (int i = thread0->data_n - 1; i >= 0; i--) {
                    if (thread0->data[i] != 0) {
                        last_non_zero = thread0->data[i];
                        break;
                    }
                }
                thread0->data[move_index] = last_non_zero;
                thread1->data_n++;
                thread0->data_n--;
            }
        }



        int extend_high = values_low_to_high - values_high_to_low;
        int extend_low = values_high_to_low - values_low_to_high;



    }
    #pragma omp parallel
    {
        #pragma omp task 
        {
            global_sort(thread_data, group_thread_start, group_thread_start + group_size / 2);
        }
        #pragma omp task 
        {
            global_sort(thread_data, group_thread_start + group_size / 2, group_thread_end);
        }
    }
}

int quick_sort_parallel(thread_data_t ** thread_data, int threads_n) {
    int * thread_medians = (int *)malloc(threads_n * sizeof(int));
    #pragma omp parallel for schedule(dynamic) num_threads(threads_n)
    for (int tid = 0; tid < threads_n; ++tid) {
        quick_sort_local(thread_data[tid]->data, thread_data[tid]->data_n);
    }
    global_sort(thread_data, 0, threads_n);
    #pragma omp parallel for schedule(dynamic) num_threads(threads_n)
    for (int tid = 0; tid < threads_n; ++tid) {
        quick_sort_local(thread_data[tid]->data, thread_data[tid]->data_n);
    }
}

bool validate(thread_data_t ** thread_data, int threads_n) {
    for(int tid = 0; tid < threads_n; tid++) {
        if(tid > 0) {
            if(thread_data[tid-1]->data[thread_data[tid-1]->data_n-1] > thread_data[tid]->data[0]) {
                printf("Error: %d > %d\n", thread_data[tid-1]->data[thread_data[tid]->data_n-1], thread_data[tid]->data[0]);
                return false;
            }
        }
        for(int i = 0; i < thread_data[tid]->data_n-1; i++) {
            if(thread_data[tid]->data[i] > thread_data[tid]->data[i+1]) {
                return false;
            }
        }
    }
    return true;
}

void setup_threads() {
}

int main() {

    int n, num_threads;
    // Random data
    // printf("Enter the number of data to be sorted (in millions): ");
    // scanf("%d", &n);
    // printf("Enter the number of threads: ");
    // scanf("%d", &threads);
    n = 2000;
    num_threads = 8;
    n *= 1000;
    int data_n_per_thread = n / num_threads;

    thread_data_t **thread_data =
        (thread_data_t **)malloc(num_threads * sizeof(thread_data_t *));
    for (int tid = 0; tid < num_threads; tid++) {
        thread_data[tid] = (thread_data_t *)malloc(sizeof(thread_data_t));
        thread_data[tid]->data =
            (int *)malloc(data_n_per_thread * 2 * sizeof(int));
        thread_data[tid]->data_n = data_n_per_thread;
        for (int i = 0; i < data_n_per_thread; i++) {
            thread_data[tid]->data[i] = rand() % 900+100;
        }
    }
    //Set omp threads
    omp_set_num_threads(num_threads);
    quick_sort_parallel(thread_data, num_threads);
    printf("\n");
    /*bool valid = validate(thread_data, num_threads);
    if(valid) {
        printf("Valid\n");
    } else {
        printf("Invalid\n");
    }*/

    printf("\n");
    


    return 0;
}
