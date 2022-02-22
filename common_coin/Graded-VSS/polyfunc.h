#ifndef POLYFUNC_H
#define POLYFUNC_H

#include "globals.h"
#include <gsl/gsl_poly.h>
#include <math.h>

void printEvaluatedPolys(int numOfNodes, double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double RootPoly[]);
void evaluatePolynomials(int badplayers,
						double polynomials[][CONFIDENCE_PARAM][badplayers],
						double polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						double RootPoly[],
						double EvaluatedRootPoly[]);
void printPolynomials(int badplayers, double polynomials[][CONFIDENCE_PARAM][badplayers], double RootPoly[]);
void GenerateRandomPoly(int badplayers, double polynomials[][CONFIDENCE_PARAM][badplayers], double RootPoly[]);
void printRootPolyOnly(double RootPoly[]);

#endif
