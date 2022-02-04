#ifndef INIT_H
#define INIT_H

#include "globals.h"

void init(void *context, 
		struct servers reqServer[],
		char serversIP[][256],
		double polynomials[][CONFIDENCE_PARAM][badPlayers],
		double polyEvals[][numOfNodes][CONFIDENCE_PARAM],
		double RootPoly[],
		double EvaluatedRootPoly[],
		double RootPolynomial[]);

void ReadIPFromFile(char serversIP[][256], char *filename);
void PrepareConnections(void *context, struct servers reqServer[], char serversIP[][256]);
void ValidateInput(int argc);

#endif