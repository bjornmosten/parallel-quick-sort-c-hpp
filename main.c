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

typedef struct processor_data {
    int *data;
    int data_n;
    int split_index;
} processor_data_t;

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

int median(processor_data_t *processor_data) {
    int *data = processor_data->data;
    int data_n = processor_data->data_n;
    return data[data_n / 2];
}

void print_array(processor_data_t **processor_data, int processors_n) {
    for (int tid = 0; tid < processors_n; tid++) {
        printf("processor %d: ", tid);
        for (int i = 0; i < processor_data[tid]->data_n; i++) {
            printf("%d ", processor_data[tid]->data[i]);
        }
        printf("\n");
    }
    printf("\n");
}

void move_values(processor_data_t *__restrict processor_0,
                 int move_from_0_start, int move_from_0_end,
                 processor_data_t *__restrict processor_1,
                 int move_from_1_start, int move_from_1_end, processor_data_t ** all_p) {
    int swap_n = move_from_1_end - move_from_1_start;
    for (int hi = 0; hi < swap_n; hi++) {
        int index_0 = move_from_0_start + hi;
        int index_1 = move_from_1_start + hi;
        printf("Swapping indexes %d and %d with values %d and %d\n", index_0,
               index_1, processor_0->data[index_0], processor_1->data[index_1]);
        int value = processor_1->data[index_1];
        processor_1->data[index_1] = processor_0->data[index_0];
        processor_0->data[index_0] = value;
        print_array(all_p, 4);
    }
    int move_to_1_n = (move_from_0_end - move_from_0_start) - swap_n;
    for (int li = 0; li < move_to_1_n; li++) {
        int move_index = swap_n + li;
        processor_0->data[processor_0->data_n] = processor_1->data[move_index];
        processor_1->data[move_index] =
            processor_1->data[processor_1->data_n - 1];
        processor_0->data_n--;
        processor_1->data_n++;
    }
}

// Find the split point. Assumes that the values are sorted
int find_split(processor_data_t ** all_processors, int current_processor_id,
                int group_start, int group_size) {
    int median_sum = 0;
    for (int i = 0; i < group_size; i++) {
        median_sum += median(all_processors[i+group_start]);
    }
    int group_pivot = median_sum / group_size;
    int split_index = 0;
    processor_data_t * current_processor = all_processors[current_processor_id];
    for (int i = 0; i < current_processor->data_n; i++) {
        if (current_processor->data[i] > group_pivot) {
            split_index = i;
            break;
        }
    }
    current_processor->split_index = split_index;
    return group_pivot;
}

void global_sort(processor_data_t **all_processors, int group_start,
                 int group_size) {
    if (group_size <= 1) {
        return;
    }
    for (int i = 0; i < group_size; i++) {
        int partner_processor_id = group_start + group_size - i - 1;
        printf("Current processor: %d, partner processor: %d\n",
               i + group_start, partner_processor_id);
        int pivot = find_split(all_processors, i + group_start, group_start,
                               group_size);
        printf("Pivot: %d\n", pivot);


    }
#pragma omp taskwait
    for (int i = 0; i < group_size / 2; i++) {
        print_array(all_processors, 4);
        int current_processor_id = i + group_start;
        int partner_processor_id = group_start + group_size - i - 1;
        printf("Current processor: %d, partner processor: %d\n",
               current_processor_id, partner_processor_id);
        processor_data_t *current_processor =
            all_processors[current_processor_id];
        processor_data_t *partner_processor =
            all_processors[partner_processor_id];
        int move_from_low_start = current_processor->split_index;
        int move_from_low_end = current_processor->data_n;
        int move_from_high_start = 0;
        int move_from_high_end = partner_processor->split_index;
        // If more values are going to be moved from low to high, swap
        // values and then perform transfers which increase or decrease
        if (move_from_low_end - move_from_low_start >
            move_from_high_end - move_from_high_start) {
            move_values(current_processor, move_from_low_start,
                        move_from_low_end, partner_processor,
                        move_from_high_start, move_from_high_end, all_processors);
        } else {
            move_values(partner_processor, move_from_high_start,
                        move_from_high_end, current_processor,
                        move_from_low_start, move_from_low_end, all_processors);
        }
        qsort(current_processor->data, current_processor->data_n, sizeof(int),
              cmpfunc);
        qsort(partner_processor->data, partner_processor->data_n, sizeof(int),
              cmpfunc);
    }
    global_sort(all_processors, group_start, group_size / 2);
    global_sort(all_processors, group_start + group_size / 2, group_size / 2);
}

int quick_sort_parallel(processor_data_t **processor_data, int processors_n) {

    for (int tid = 0; tid < processors_n; ++tid) {
#pragma omp task
        qsort(processor_data[tid]->data, processor_data[tid]->data_n,
              sizeof(int), cmpfunc);
    }
#pragma omp taskwait
    printf("Global sort start\n");
    global_sort(processor_data, 0, processors_n);
    printf("Global sort done\n");
    return 0;
}

bool validate(processor_data_t **processor_data, int processors_n) {
    for (int tid = 0; tid < processors_n; tid++) {
        if (tid > 0) {
            if (processor_data[tid - 1]
                    ->data[processor_data[tid - 1]->data_n - 1] >
                processor_data[tid]->data[0]) {
                printf("Error: %d > %d\n",
                       processor_data[tid - 1]
                           ->data[processor_data[tid]->data_n - 1],
                       processor_data[tid]->data[0]);
                return false;
            }
        }
        for (int i = 0; i < processor_data[tid]->data_n - 1; i++) {
            if (processor_data[tid]->data[i] >
                processor_data[tid]->data[i + 1]) {
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

void setup_processors() {}

int main() {

    int n, num_procs;
    // Random data
    // printf("Enter the number of data to be sorted (in millions): ");
    // scanf("%d", &n);
    // printf("Enter the number of processors: ");
    // scanf("%d", &processors);
    n = 2;
    num_procs = 4;
    n *= 10;
    int randmax = (int)1E2;
    int data_n_per_processor = n / num_procs;
    int num_threads = num_procs;

    omp_set_dynamic(0);
    omp_set_nested(1);
    omp_set_num_threads(num_threads);
    srand(52151);

#if USE_BUILTIN_QSORT
    int *data = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        data[i] = rand() % randmax + 1;
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

    processor_data_t **processor_data =
        (processor_data_t **)malloc(num_procs * sizeof(processor_data_t *));
    void *processor_data_mem = malloc(num_procs * sizeof(processor_data_t));
    for (int tid = 0; tid < num_procs; tid++) {
        processor_data[tid] = (processor_data_t *)processor_data_mem + tid;
        processor_data[tid]->data =
            (int *)malloc(data_n_per_processor * 2 * sizeof(int));
        processor_data[tid]->data_n = data_n_per_processor;
        for (int i = 0; i < data_n_per_processor; i++) {
            processor_data[tid]->data[i] = rand() % randmax + 1;
        }
    }

    print_array(processor_data, num_procs);
#pragma omp parallel num_threads(num_procs)
    {
#pragma omp single nowait
        quick_sort_parallel(processor_data, num_procs);
    }
    print_array(processor_data, num_procs);
    printf("\n");
    bool valid = validate(processor_data, num_procs);
    if (valid) {
        printf("Valid\n");
    } else {
        printf("Invalid\n");
    }

    printf("\n");
    return 0;
}
