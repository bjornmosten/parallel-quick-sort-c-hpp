all:
	gcc -o main main.c -lpthread -lm -fopenmp -Ofast -march=native
	time ./main
