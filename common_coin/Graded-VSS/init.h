#ifndef INIT_H
#define INIT_H

#include "globals.h"

void init(void *context, 
		struct servers reqServer[], 
		struct servers syncServer[], 
		char serversIP[][256], 
		char syncIP[][256], 
		int polynomials[][CONFIDENCE_PARAM][badPlayers], 
		int polyEvals[][numOfNodes][CONFIDENCE_PARAM],
		int RootPoly[],
		int EvaluatedRootPoly[]);

void ReadIPFromFile(char serversIP[][256], char *filename);
void PrepareConnections(void *context, struct servers reqServer[], char serversIP[][256]);
void ValidateInput(int argc);

#endif