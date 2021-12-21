#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "globals.h"

void Distribute(struct servers reqServer[], const char *commonString);
void GetMessages(struct servers reqServer[], const char *commonString);
char *GetFromDistributor(struct servers reqServer[], int distributor);
void DistributorDistribute(struct servers reqServer[], const char *secret, int distributor);
void GradeCast(struct servers reqServer[], struct servers syncServer[], int distributor, char *message);
void WaitForDealerSignal(struct servers syncServer[]);
void SimpleGradedDecide(struct servers reqServer[], struct servers syncServer[], char *secret);

#endif