#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <assert.h>
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

struct output
{
	int code;
	int value;
};

struct servers{
	int type;
	void *value;
};

//Global Variables. This are global since they are for the whole process
extern  int numOfNodes;
extern  int dealer;
extern  int tally;
extern  int proc_id;
extern  int badPlayers;
extern  struct output out;

//use this bad boy so printf are printed on demand and not always. fflush is to force the output in case we write to file through bash
#ifdef DEBUG
#define TraceDebug(fmt, ...) fprintf(stdout,"DEBUG " "%s %d " fmt, GetTime(), getpid(), ##__VA_ARGS__); fflush(stdout)
#else
#define TraceDebug(fmt, ...) ((void)0)
#endif
//use this bad boy instaed of printf for better formatting. fflush is to force the output in case we write to file through bash
#define TraceInfo(fmt, ...)	fprintf(stdout,"INFO  " "%s %d " fmt, GetTime(), getpid(), ##__VA_ARGS__); fflush(stdout)

char *GetTime();
void Distribute(struct servers reqServer[], const char *secret);
void GetMessages(struct servers reqServer[], const char *secret);
void ValidateTally();
void PrepareConnections(void *context, struct servers reqServer[], char serversIP[][256]);
char *GetFromDealer(struct servers reqServer[]);
void DealerDistribute(struct servers reqServer[], const char *secret);
void init(char serversIP[][256]);
void ValidateInput(int argc);

#endif