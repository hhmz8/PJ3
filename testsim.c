/*
Hanzhe Huang
10/20/2021
testsim.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h> //isprint
#include <unistd.h> //sleep
#define OUT_FILE "logfile"

int main(int argc, char** argv){
	FILE* fptr;
	fptr = fopen(OUT_FILE, "a");
	if (argc != 3) {
		perror("Error: Testsim requires 2 arguments.");
		return -1;
	}
	pid_t pid = getpid();
	int sleepTime = atoi(argv[1]);
	int repeat = atoi(argv[2]);
	
	int i;
	for (i = 0; i < repeat; i++){
		sleep(sleepTime);
		char timeBuffer[40];
		time_t tempTime = time(NULL);
		strftime(timeBuffer, 40, "%H:%M:%S", localtime(&tempTime));
		printf("Saving: %s %d %d of %d iterations\n", timeBuffer, pid, (i+1), repeat);
		fprintf(fptr, "%s %d %d of %d iterations\n", timeBuffer, pid, (i+1), repeat);
	}
	fclose(fptr);
	return 0;
}