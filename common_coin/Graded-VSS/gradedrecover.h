#ifndef GRADERECOVER_H
#define GRADERECOVER_H

#include "globals.h"
#include "gradecast.h"

void SimpleGradedRecover(struct servers reqServer[], 
						double Secret_hj[][numOfNodes],
						struct output candidate[],
						int tally[]);

#endif