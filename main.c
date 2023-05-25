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

// Linked list
typedef struct ll_node {
    int value;
    struct ll_node *next;
} ll_node;

typedef struct ll {
    ll_node *head;
    ll_node *tail;
} ll;

// Linked list functions

ll_node *create_node(int value) {
    ll_node *node = (ll_node *)malloc(sizeof(ll_node));
    node->value = value;
    node->next = NULL;
    return node;
}

ll_node * insert_node(ll * data, int value) {
    ll_node *node = create_node(value);
    if(data->head == NULL) {
        data->head = node;
        data->tail = node;
        return node;
    }
    data->tail->next = node;
    data->tail = node;
    return node;
}

void print_list(ll_node *head) {
    ll_node *current = head;
    while (current != NULL) {
        printf("%d ", current->value);
        current = current->next;
    }
    printf("\n");
}

void swap_values(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

ll_node * partition(ll_node *head, ll_node *tail, ll_node **new_head, ll_node** new_tail) {
    printf("start\n");
    ll_node *pivot = tail;
    ll_node *current = head;
    ll_node *prev = NULL;
    ll_node *end = tail;
    while (current != pivot) {
        if (current->value < pivot->value) {
            if (*new_head == NULL) {
                *new_head = current;
            }
            prev = current;
            current = current->next;
        } else {
            if (prev != NULL) {
                prev->next = current->next;
            }
            ll_node *temp = current->next;
            current->next = NULL;
            end->next = current;
            end = current;
            current = temp;
        }
    }
    if (new_head == NULL) {
        *new_head = pivot;
    }
    *new_tail = end;
    printf("end\n");
    return pivot;

}

//ERROR IS LIKELY DUE TO TAIL NOT BEING UPDATED
//
// Quick sort locally, returning the median after sorting
ll_node * quick_sort_local(ll_node *head, ll_node *tail) {
    if (!head || head == tail) {
        return head;
    }
    ll_node *new_head = NULL;
    ll_node *new_tail = NULL;
    ll_node * pivot = partition(head, tail, &new_head, &new_tail);
    ll_node *current = new_head;
    if(new_head != pivot) {
        while (current->next != pivot) {
            current = current->next;
        }
        current->next = NULL;
        new_head = quick_sort_local(new_head, current);
        current = new_head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = pivot;
    }
    pivot->next = quick_sort_local(pivot->next, new_tail);
    return new_head;
}
void print_array(int *data, int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", data[i]);
    }
    printf("\n");
}
/*
    int split_points[num_threads];
    for(int tid = 0; tid < num_threads; tid++) {
        int first = tid * data_n / num_threads;
        int last = (tid + 1) * data_n / num_threads;
        for(int j = 0; j < last-first; j++) {
            if(data[first+j] > pivot) {
                break;
            }
            split_points[tid] = j;
        }
    }
*/

void split(ll_node *head, ll_node *tail, int pivot, ll_node *low, ll_node *high) {
    ll_node *current = head;
    while (current->value < pivot && current != tail && current != NULL) {
        current = current->next;
    }
    // Split the list
    low = head;
    current->next = NULL;
    high = current;

}

void global_sort(ll ** ll_data, int pivot, int num_threads) {
    if (num_threads == 1) {
        return;
    }

    // Exchange data
    for (int g = 0; g < num_threads / 2; g++) {
        int tid = g;
        int tid_other = num_threads - tid - 1;

        return;
        ll_node *low;
        ll_node *high;
        split(ll_data[tid]->head, ll_data[tid]->tail, pivot, low, high);
        ll_node *low_other;
        ll_node *high_other;
        split(ll_data[tid_other]->head, ll_data[tid_other]->tail, pivot, low_other, high_other);
        low->next = low_other;
        high_other->next = high;
    }
}

int quick_sort_parallel(ll ** data, int group_size, int depth) {
    if (group_size <= 1) {
        return 0;
    }
    int thread_medians[group_size];
    for (int tid = 0; tid < group_size; ++tid) {
        thread_medians[tid] = quick_sort_local(data[tid]->head, data[tid]->tail);
    }
    int median_avg = 0;
    for (int i = 0; i < group_size; i++) {
        median_avg += thread_medians[i];
    }
    median_avg /= group_size;
    global_sort(data, median_avg, group_size);

}

bool validate(ll * data) {
    ll_node *current = data->head;
    while (current->next != NULL) {
        if (current->value > current->next->value) {
            return false;
        }
        current = current->next;
    }
    return true;
}

int main() {

    int n, num_threads;
    // Random data
    // printf("Enter the number of data to be sorted (in millions): ");
    // scanf("%d", &n);
    // printf("Enter the number of threads: ");
    // scanf("%d", &threads);
    n = 1;
    num_threads = 4;
    n *= 20;
    ll ** data_threads = (ll **)malloc(num_threads * sizeof(ll *));
    for (int i = 0; i < num_threads; i++) {
        data_threads[i] = (ll *)malloc(sizeof(ll));
        for(int ni = 0; ni < n; ni++) {
            insert_node(data_threads[i], rand() % 900 + 100);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        print_list(data_threads[i]->head);
    }
    printf("\n");
    quick_sort_parallel(data_threads, num_threads, 0);
    for (int i = 0; i < num_threads; i++) {
        print_list(data_threads[i]->head);
    }
    


    return 0;
}
