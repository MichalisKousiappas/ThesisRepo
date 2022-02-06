#ifndef GRADEDSHARE_H
#define GRADEDSHARE_H

#include "globals.h"
#include "polyfunc.h"

char *SimpleGradedShare(struct servers syncServer[], gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM], gsl_complex EvaluatedRootPoly[]);
int ParseSecret(char *secret, gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM], gsl_complex EvaluatedRootPoly[]);

#endif