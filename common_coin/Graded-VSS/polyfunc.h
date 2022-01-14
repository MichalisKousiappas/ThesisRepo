#ifndef POLYFUNC_H
#define POLYFUNC_H

#include "globals.h"
#include <math.h>

void printEvaluatedPolys(int numOfNodes, int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int RootPoly[]);
void evaluatePolynomials(int badplayers, int polynomials[][CONFIDENCE_PARAM][badplayers], int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int RootPoly[], int EvaluatedRootPoly[]);
void printPolynomials(int badplayers, int polynomials[][CONFIDENCE_PARAM][badplayers], int RootPoly[]);
void GenerateRandomPoly(int badplayers, int polynomials[][CONFIDENCE_PARAM][badplayers], int RootPoly[]);

#endif
