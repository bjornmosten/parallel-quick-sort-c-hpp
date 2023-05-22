#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
//openmp
#include <omp.h> 

#define BOARD_SIZE 9


int** setup_board(int size) {
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

//Check for duplicate numbers (excluding 0) in board array
bool validate_board(int** board, int size) {
    int i, j, k;
    //Check rows
    for(i = 0; i < size; i++) {
        int row[BOARD_SIZE] = {0};
        for(j = 0; j < size; j++) {
            if(board[i][j] != 0) {
                if(row[board[i][j]-1] == 0) {
                    row[board[i][j]-1] = 1;
                } else {
                    return false;
                }
            }
        }
    }
    //Check columns
    for(i = 0; i < size; i++) {
        int col[BOARD_SIZE] = {0};
        for(j = 0; j < size; j++) {
            if(board[j][i] != 0) {
                if(col[board[j][i]-1] == 0) {
                    col[board[j][i]-1] = 1;
                } else {
                    return false;
                }
            }
        }
    }
    //Check squares
    for(i = 0; i < size; i+=3) {
        for(j = 0; j < size; j+=3) {
            int square[BOARD_SIZE] = {0};
            for(k = 0; k < size; k++) {
                if(board[i + (k/3)][j + (k%3)] != 0) {
                    if(square[board[i + (k/3)][j + (k%3)]-1] == 0) {
                        square[board[i + (k/3)][j + (k%3)]-1] = 1;
                    } else {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool solve_board(int **board, int unassigned_start, int unassigned_end) {
    if(unassigned_start >= unassigned_end) {
        return true;
    }
    int i = unassigned_start / BOARD_SIZE;
    int j = unassigned_start - (i * BOARD_SIZE);
    for(int k = 1; k <= BOARD_SIZE; k++) {
        board[i][j] = k;
        if(validate_board(board, BOARD_SIZE)) {
            if(solve_board(board, unassigned_start+1, unassigned_end)) {
                return true;
            }
        }
    }
    board[i][j] = 0;
    return false;
}








int main() {
    int ** board = setup_board(9);
    solve_board(board, 0, 20);
    printf("Solved board:\n");
    print_board(board, 9);
    return 0;
}
