#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "globals.h"

void Distribute(struct servers reqServer[], const char *secret);
void GetMessages(struct servers reqServer[], const char *secret);
void ValidateTally();
char *GetFromDistributor(struct servers reqServer[], int distributor);
void DistributorDistribute(struct servers reqServer[], const char *secret, int distributor);
void GradeCast(struct servers reqServer[], struct servers syncServer[], int distributor);

#endif