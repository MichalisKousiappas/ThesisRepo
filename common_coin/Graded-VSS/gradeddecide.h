#ifndef GRADEDDECIDE_H
#define GRADEDDECIDE_H

#include "globals.h"

void SimpleGradedDecide(struct servers reqServer[],
						double polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						double EvaluatedRootPoly[],
						double polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						double RootPolynomial[badPlayers]);

void Distribute(struct servers reqServer[], const char *commonString);
char *GetFromDistributor(struct servers reqServer[], int distributor);
void DistributorDistribute(struct servers reqServer[], const char *secret, int distributor);
char *GradeCast(struct servers reqServer[], int distributor, const char *message);
int ParseSecret(char *secret, double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double EvaluatedRootPoly[]);
void SimpleGradedRecover(struct servers reqServer[], double RootPolynomial[]);

#endif