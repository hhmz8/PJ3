/*
Hanzhe Huang
10/20/2021
runsim.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h> //time
#include <sys/wait.h> //wait
#include <signal.h>
#include <ctype.h> //isprint
#include <unistd.h> //sleep, alarm

#include "runsim.h"

// Reference: https://www.tutorialspoint.com/inter_process_communication/inter_process_communication_shared_memory.htm
// Reference: https://stackoverflow.com/questions/19461744/how-to-make-parent-wait-for-all-child-processes-to-finish

extern int errno;

struct shmseg {
   int nlicenses;
   int avaliable[MAX_PRO]; //Boolean
   int choosing[MAX_PRO]; //Boolean
   int numbers[MAX_PRO]; //Turn #
   char buf[BUF_SIZE];
};

int main(int argc, char** argv) {
	// Signal handlers;
	signal(SIGINT, sigint_parent);
	signal(SIGALRM, sigalrm);
	alarm(MAX_TIME);
	
	FILE* fptr;
	int i;
	int id = 0;
	int pid = 1;
	int option = 0;
	int licenseLimit = 0;
	char arg[3][MAX_CHAR];
	fscanf(stdin, "%s", arg[0]);
	fscanf(stdin, "%s", arg[1]);
	fscanf(stdin, "%s", arg[2]);
	
	// Clear log file
	fptr = fopen(OUT_FILE, "w");
	fclose(fptr);

	// Reference: https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
	while ((option = getopt(argc, argv, "ht:")) != -1) {
		switch (option) {

		case '?':
			if (isprint(optopt))
				perror("Unknown option");
			else
				perror("Unknown option character");
			return -1;
			break;
		}
	}
	
	if (argc > 2) {
		perror("Too many parameters entered");
		return -1;
	}
	if (optind < argc) {
		licenseLimit = atoi(argv[optind]);
		if (licenseLimit > MAX_PRO - 2){
			perror("License number too large");
			return -1;
		}
	}
	else {
		printf("Defaulting license # to 1.\n");
		licenseLimit = 1;
	}
	
	// Init 
	initlicense(license(), licenseLimit);
	
	// Fork loop
	while(1){
		// Request license
		getlicense(license());
		removelicenses(license(),1);
		printf("Fetched license. Remaining licenses: %d\n", license()->nlicenses);
		license()->avaliable[id] = 0;
		
		pid = fork();
		switch ( pid )
		{
		case -1:
			perror("Error: fork");
			return -1;

		case 0: // Child, terminates
			child(id, arg[0], arg[1], arg[2]);
			break;

		default: // Parent, loops
			// Get empty bakery id
			for (i = 0; i < MAX_PRO; i++){
				if (license()->avaliable[i] == 1){
					id = i;
					break;
				}
			}
			// Get next inputs
			for (i = 0; i < 3; i++){
				if (fscanf(stdin, "%s", arg[i]) == EOF){
					// EOF 
					printf("End of file reached.\n");
					parent();
					deallocate();
					logexit();
					return 0;
				}
			}
			break;
		}
	}
	logexit();
	return -1;
}

// Logs termination time
void logexit(){
	FILE* fptr;
	char timeBuffer[40];
	time_t tempTime = time(NULL);
	fptr = fopen(OUT_FILE, "a");
	strftime(timeBuffer, 40, "%H:%M:%S", localtime(&tempTime));
	fprintf(fptr, "%s %d terminated\n", timeBuffer, getpid());
	fclose(fptr);
}

void sigint_parent(int sig){
	printf("Process %d exiting...\n",getpid());
	deallocate();
	printf("Terminating child processes...\n");
	kill(0, SIGINT);
	parent();
	logexit();
	exit(0);
}

void sigint(int sig){
	kill(0, SIGINT);
	exit(0);
}

void sigalrm(int sig){
	printf("Program timed out.\n");
	deallocate();
	printf("Terminating child processes...\n");
	kill(0, SIGINT);
	parent();
	logexit();
	exit(0);
}

// Parent function to wait for children processes
void parent(){
	printf("Waiting for children...\n");
	int childpid;
	while ((childpid = (wait(NULL))) > 0);
	printf("Stopped waiting for children.\n");
}

// Reference: http://www.cs.umsl.edu/~sanjiv/classes/cs4760/src/shm.c
// Reference: https://www.geeksforgeeks.org/signals-c-set-2/
void child(int id, char* arg1, char* arg2, char* arg3){
	signal(SIGINT, sigint);
	signal(SIGALRM, SIG_IGN);
	
	printf("Child %d with id # %d forked from parent %d.\n",getpid(), id, getppid());
	
	// Shared memory
	struct shmseg *shmp;
    int shmid = shmget(SHM_KEY, BUF_SIZE, 0666|IPC_CREAT);
	if (shmid == -1) {
		perror("Error: shmget");
		exit(-1);
	}

	shmp = shmat(shmid, 0, 0);

	// Reference: Lecture video, https://www.geeksforgeeks.org/bakery-algorithm-in-process-synchronization/
	// Bakery algorithm 
	int i;
	int j;
	int max_number = shmp->numbers[0];
	shmp->choosing[id] = 1;
	for (i = 0; i < MAX_PRO; i++) {
		if (shmp->numbers[i] > max_number){
			max_number = shmp->numbers[i];
		}
	}
	shmp->numbers[id] = max_number + 1;
	shmp->choosing[id] = 0;
	for (j = 0; j < MAX_PRO; j++) {
		//printf("Child %d with # of %d, testing process %d with # of %d. While: %d \n",getpid(), shmp->numbers[id], j, shmp->numbers[j], (shmp->numbers[j] < shmp->numbers[id]));
		while (shmp->choosing[j] == 1);
		while ((shmp->numbers[j] != 0) && (shmp->numbers[j] < shmp->numbers[id] || (shmp->numbers[j] == shmp->numbers[id] && j < id)));
	}
	
	// Critical section
	docommand(arg1, arg2, arg3);
	// Critical secion, end 
	shmp->numbers[id] = 0;
	shmp->avaliable[id] = 1;
	
	// Return license
	returnlicense(license());
	printf("Child %d finished.\n", getpid());
	exit(0);
}

// Deallocates shared memory
void deallocate(){
	struct shmseg *shmp;
    int shmid = shmget(SHM_KEY, BUF_SIZE, 0666|IPC_CREAT);
	if (shmid == -1) {
		perror("Error: shmget");
		exit(-1);
	}
	shmp = shmat(shmid, 0, 0);
	
	if (shmdt(shmp) == -1) {
		perror("Error: shmdt");
		exit(-1);
	}
	if (shmctl(shmid, IPC_RMID, 0) == -1) {
		perror("Error: shmctl");
		exit(-1);
	}
	printf("Shared memory deallocated.\n");
}

// Returns the shared memory segment / license object
struct shmseg* license(){
	struct shmseg *shmp;
    int shmid = shmget(SHM_KEY, BUF_SIZE, 0666|IPC_CREAT);
	if (shmid == -1) {
		perror("Error: shmget");
		exit(-1);
	}
	shmp = shmat(shmid, 0, 0);
	return shmp;
}

// Blocks process until a license is avaliable
void getlicense(struct shmseg* shmp){
	if (shmp->nlicenses < 1){
		printf("%d waiting for avaliable license...\n", getpid());
		while(shmp->nlicenses < 1);
	}
}

// Increments license #
void returnlicense(struct shmseg* shmp){
	shmp->nlicenses++;
}

// Initializes license object with n licenses
void initlicense(struct shmseg* shmp, int n){
	shmp->nlicenses = n;
	int i;
	for (i = 0; i < MAX_PRO; i++) {
		shmp->avaliable[i] = 1;
		shmp->choosing[i] = 0;
		shmp->numbers[i] = 0;
	}
}

// Increments n licenses
void addtolicenses(struct shmseg* shmp, int n){
	shmp->nlicenses += n;
}

// Decrements n licenses
void removelicenses(struct shmseg* shmp, int n){
	shmp->nlicenses -= n;
}

// Runs testsim
void docommand(char* arg1, char* arg2, char* arg3){
	int pid;
	int childpid;
	
	pid = fork();
	switch ( pid )
	{
	case -1:
		perror("Error: fork");
		exit(-1);

	case 0:
		if ((execl(arg1, "docommand", arg2, arg3, (char*)NULL)) == -1){
			perror("Error: execl/stdin");
		}
		exit(-1);
		break;

	default:
		break;
	}
	
	// Wait for grandchild execl 
	while ((childpid = (wait(NULL))) > 0);
}