CC = gcc
CFLAGS = -Wall
OBJ = testsim.o runsim.o

all: runsim testsim

runsim.o: runsim.c runsim.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
testsim.o: testsim.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
runsim: runsim.o
	$(CC) $(CFLAGS) -o $@ $^
	
testsim: testsim.o
	$(CC) $(CFLAGS) -o $@ $^
	
clean:
	rm *.h *.c *.o