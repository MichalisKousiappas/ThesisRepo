#ifndef GRADEDSHARE_H
#define GRADEDSHARE_H

#include "globals.h"

char *SimpleGradedShare(struct servers syncServer[], double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double EvaluatedRootPoly[]);

#endif