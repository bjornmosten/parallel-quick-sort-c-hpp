#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <omp.h>


void swap_values(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int get_partition_index(int* data, int first, int last) {
    int pivot = data[last];
    int partition_index = first;
    for(int i = first; i < last; i++) {
        if(data[i] <= pivot) {
            swap_values(&data[i], &data[partition_index]);
            partition_index++;
        }
    }
    swap_values(&data[partition_index], &data[last]);
    return partition_index;

}

void quick_sort(int *data, int first, int last) {
    if(first < last) {
        int partition_index = get_partition_index(data, first, last);
        quick_sort(data, first, partition_index - 1);
        quick_sort(data, partition_index + 1, last);
    }
}



int main() {
    int n, threads;
    //Random data
    //printf("Enter the number of data to be sorted (in millions): ");
    //scanf("%d", &n);
    //printf("Enter the number of threads: ");
    //scanf("%d", &threads);
    n = 1;
    threads = 1;
    n *= 10000;
    int *data = (int *)malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        data[i] = rand() % 1000000;
    }
    //Sort
    double start = omp_get_wtime();
    quick_sort(data, 0, n);
    printf("Sorted data: ");
    for (int i = 0; i < 100; i++) {
        printf("%d \n", data[i]);
    }
    double end = omp_get_wtime();
    printf("Time: %lf\n", end - start);
    return 0;
}

