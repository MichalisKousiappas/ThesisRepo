#ifndef GRADECAST_H
#define GRADECAST_H

#include "globals.h"

char *GradeCast(struct servers reqServer[], int distributor, const char *message);
char *GradeCastPhaseA(struct servers reqServer[], int distributor, const char *message);
int CountSameMessageAgain(struct servers reqServer[], const char *message, int check);
int CountSameMessage(struct servers reqServer[], const char *message);
struct output ValidateTally(int tally);

#endif