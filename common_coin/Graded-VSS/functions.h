#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "globals.h"

void SimpleGradedDecide(struct servers reqServer[],
						struct servers syncServer[],
						int polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						int EvaluatedRootPoly[],
						int polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						int RootPolynomial[badPlayers]);

void Distribute(struct servers reqServer[], const char *commonString);
char *GetFromDistributor(struct servers reqServer[], int distributor);
void DistributorDistribute(struct servers reqServer[], const char *secret, int distributor);
char *GradeCast(struct servers reqServer[], struct servers syncServer[], int distributor, const char *message);
void ParseSecret(char *secret, int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int EvaluatedRootPoly[]);

#endif