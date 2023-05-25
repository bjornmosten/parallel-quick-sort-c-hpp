#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
// openmp
#include <omp.h>

#define BOARD_SIZE 9

int **setup_board(int size) {
    int **board = (int **)malloc(size * sizeof(int *));
    int i, j;
    for (i = 0; i < size; i++) {
        board[i] = (int *)malloc(size * sizeof(int));
        for (j = 0; j < size; j++) {
            board[i][j] = 0;
        }
    }
    return board;
}

void print_board(int **board, int size) {
    int i, j;
    for (i = 0; i < size; i++) {
        printf("|");
        for (j = 0; j < size; j++) {
            printf("%d|", board[i][j]);
        }
        printf("\n");
    }
}

// Check for duplicate numbers (excluding 0) in board array
bool validate_board(int **board, int check_row, int check_col, int new_val) {
    int seen[BOARD_SIZE] = {0};
    for (int i = 0; i < BOARD_SIZE; i++) {
        if(i == check_col) {
            if(new_val != 0) {
                if (seen[new_val - 1] == 1) {
                    return false;
                }
                seen[new_val - 1] = 1;
            }
        }
        if (board[check_row][i] != 0) {
            if (seen[board[check_row][i] - 1] == 1) {
                return false;
            }
            seen[board[check_row][i] - 1] = 1;
        }
    }
    memset(seen, 0, sizeof(seen));
    for (int i = 0; i < BOARD_SIZE; i++) {
        if(i == check_row) {
            if(new_val != 0) {
                if (seen[new_val - 1] == 1) {
                    return false;
                }
                seen[new_val - 1] = 1;
            }
        } 
        if (board[i][check_col] != 0) {
            if (seen[board[i][check_col] - 1] == 1) {
                return false;
            }
            seen[board[i][check_col] - 1] = 1;
        }
    }
    board[check_row][check_col] = new_val;
    return true;
}

bool solve_board(int **board, int unassigned_start, int unassigned_end, int ** solved_board) {
    int ** local_board = setup_board(BOARD_SIZE);
    memcpy(local_board, board, BOARD_SIZE * BOARD_SIZE * sizeof(int));
    print_board(local_board, BOARD_SIZE);
    if (unassigned_start > unassigned_end) {
        printf("Unassigned start is greater than unassigned end\n");
        return false;
    }
    int row = unassigned_start / BOARD_SIZE;
    int col = unassigned_start % BOARD_SIZE;
    bool solved = false;
    for (int k = 1; k <= BOARD_SIZE; k++) {
        if (validate_board(local_board, row, col, k)) {
            if (solve_board(local_board, unassigned_start + 1, unassigned_end, solved_board)) {
                memcpy(solved_board, local_board, BOARD_SIZE * BOARD_SIZE * sizeof(int));
                return true;
            }
        }
    }
    if (solved) {
        return true;
    }
    return false;
}

int main() {
    int **board = setup_board(BOARD_SIZE);
    int **solved_board = setup_board(BOARD_SIZE);
    solve_board(board, 0, BOARD_SIZE * BOARD_SIZE, solved_board);
    printf("Solved board:\n");
    print_board(solved_board, BOARD_SIZE);
    return 0;
}
