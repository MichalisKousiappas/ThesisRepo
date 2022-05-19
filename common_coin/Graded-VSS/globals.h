#ifndef GLOBALS_H
#define GLOBALS_H

#include <assert.h>
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>

#define ALL_MESSAGE_DELIMITERS "|:"
#define MESSAGE_DELIMITER "|" // The common delimiter in messages to separete values
#define MESSAGE_ACCEPT ":"
#define TRAITORS 0 // Controls whether bad processes will be ON (simulated)
#define DEAD_NODES 0 // Controls whether bad processes will die randomly

/*
 Used to control how big the coeficient of the polyonims can go.
 Above 2.5k the precision in CheckForGoodPiece must be increased to 0.01 to work.
 Keep it low so its easy to follow
 
 Biggest value that works with PRECISSION as 0.1
 #define MAX_COEFICIENT (INT_MAX/10000)
*/
#define MAX_COEFICIENT 100

// limit the precision check to 3 bits instead of 4. This is because the calculations can't be this precise
#define PRECISSION 0.001

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define TIMEOUT_MULTIPLIER 200

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
extern int numOfNodes;
extern int dealer;
extern int proc_id;
extern int badPlayers;
extern struct output *outArray;
extern int messages;
extern int maxNumberOfMessages;
extern int StringSecreteSize;
extern int PrimeCongruent;
extern double RootOfUnity;
extern char TimeVar[25];
extern int CONFIDENCE_PARAM;
extern int *TimedOut;

//use this bad boy so printf are printed on demand and not always. fflush is to force the output in case we write to file through bash
#ifdef DEBUG
#define TraceDebug(fmt, ...) do{ GetTime(TimeVar); fprintf(stdout,"DEBUG " "%s %d " fmt, TimeVar, getpid(), ##__VA_ARGS__); fflush(stdout);}while(0)
#else
#define TraceDebug(fmt, ...) do{((void)0);}while(0)
#endif
//use this bad boy instaed of printf for better formatting. fflush is to force the output in case we write to file through bash
#define TraceInfo(fmt, ...)	do{ GetTime(TimeVar); fprintf(stdout,"INFO  " "%s %d " fmt, TimeVar, getpid(), ##__VA_ARGS__); fflush(stdout);}while(0)

#define IsDealer (proc_id == dealer)

//Global Function declaration
void GetTime(char res[]);
void WaitForDealerSignal(struct servers syncServer[]);
void parallel_array_merge_sort(int i, int j, double a[], double aux[], double b[], double bux[]);
void Traitor(char *sendBuffer);
void Distribute(struct servers reqServer[], const char *commonString);
void TimeoutDetected(const char *Who, int node);
void RandomDeath();

#endif
