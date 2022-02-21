#ifndef GRADERECOVER_H
#define GRADERECOVER_H

#include "globals.h"

void SimpleGradedRecover(struct servers reqServer[], 
						double EvaluatedRootPoly[],
						struct output candidate[],
						int tally[]);

#endif