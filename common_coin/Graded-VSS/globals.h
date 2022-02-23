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

#define ALL_MESSAGE_DELIMITERS "|:"
#define CONFIDENCE_PARAM 5 // Controls how many polynomials will be generated
#define MESSAGE_DELIMITER "|" // The common delimiter in messages to separete values
#define MESSAGE_ACCEPT ":"
#define TRAITORS 0 // Controls whether bad processes will be ON (simulated)

/*
 Used to control how big the coeficient of the polyonims can go.
 Keep it low so its easy to follow
*/
#define MAX_COEFICIENT 100

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

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

//use this bad boy so printf are printed on demand and not always. fflush is to force the output in case we write to file through bash
#ifdef DEBUG
#define TraceDebug(fmt, ...) do{ GetTime(TimeVar); fprintf(stdout,"DEBUG " "%s %d " fmt, TimeVar, getpid(), ##__VA_ARGS__); fflush(stdout);}while(0)
#else
#define TraceDebug(fmt, ...) ((void)0)
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

#endif
