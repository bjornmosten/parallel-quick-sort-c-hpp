.PHONY: build_run 

build_run: build
	./main 40000 10000 8

build:
	gcc -o main main.c -lm -Ofast -flto -ftree-vectorize -march=native -mtune=native -fopenmp -funroll-loops  -Wno-unused-result -mno-vzeroupper \
		-fno-trapping-math -fno-signaling-nans -funsafe-math-optimizations -fno-signaling-nans -ffast-math -funroll-loops -fassociative-math -g



