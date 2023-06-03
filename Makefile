all:
	gcc -o main main.c -lm -Ofast -flto -ftree-vectorize -march=native -mtune=native -fopenmp -funroll-loops  -Wno-unused-result -mno-vzeroupper \
		-fno-trapping-math -fno-signaling-nans -funsafe-math-optimizations -fno-signaling-nans -ffast-math -funroll-loops -fassociative-math -g
	time ./main
