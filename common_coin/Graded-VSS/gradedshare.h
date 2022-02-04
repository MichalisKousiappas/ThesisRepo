#ifndef GRADEDSHARE_H
#define GRADEDSHARE_H

#include "globals.h"
#include "polyfunc.h"

char *SimpleGradedShare(struct servers syncServer[], double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double EvaluatedRootPoly[]);
int ParseSecret(char *secret, double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double EvaluatedRootPoly[]);

#endif