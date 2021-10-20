/*
Hanzhe Huang
10/20/2021
runsim.h
*/

#ifndef RUNSIM_H
#define RUNSIM_H

void logexit();
void sigint_parent(int sig);
void sigint(int sig);
void sigalrm(int sig);

void parent();
void child(int id, char* arg1, char* arg2, char* arg3);
void deallocate();
struct shmseg* license();

void getlicense(struct shmseg* shmp);
void returnlicense(struct shmseg* shmp);
void initlicense(struct shmseg* shmp, int n);
void addtolicenses(struct shmseg* shmp, int n);
void removelicenses(struct shmseg* shmp, int n);
void docommand(char* arg1, char* arg2, char* arg3);

#define BUF_SIZE 1024
#define SHM_KEY 806040
#define MSG_KEY 806041
#define MAX_CHAR 20
#define MAX_PRO 20
#define MAX_TIME 60
#define OUT_FILE "logfile"

#endif