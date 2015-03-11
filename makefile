# makefile

all: memtest

ackerman.o: ackerman.c ackerman.h 
	gcc -c -g ackerman.c -std=c99

my_allocator.o : my_allocator.c my_allocator.h
	gcc -c -g my_allocator.c -std=c99

memtest: memtest.c ackerman.o my_allocator.o
	gcc -o memtest -std=c99 memtest.c ackerman.o my_allocator.o

