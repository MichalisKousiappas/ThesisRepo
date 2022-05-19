#ifndef GRADECAST_H
#define GRADECAST_H

#include "globals.h"

void GradeCast(struct servers reqServer[], int distributor, const char *message, struct output array[], char result[]);
void GradeCastPhaseA(struct servers reqServer[], int distributor, const char *message, char result[]);
int CountSameMessageAgain(struct servers reqServer[], const char *message, int check);
int CountSameMessage(struct servers reqServer[], const char *message, int check);
struct output ValidateTally(int tally);

#endif