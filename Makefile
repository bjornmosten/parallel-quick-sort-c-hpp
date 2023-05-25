all:
	gcc -o main main.c -lpthread -lm -fopenmp
	./main
