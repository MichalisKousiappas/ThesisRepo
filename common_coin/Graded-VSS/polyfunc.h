#ifndef POLYFUNC_H
#define POLYFUNC_H

#include "globals.h"
#include <math.h>

void printEvaluatedPolys(int numOfNodes, gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM], gsl_complex EvalRootPoly[]);
void evaluatePolynomials(int badplayers, 
						double polynomials[][CONFIDENCE_PARAM][badplayers],
						gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM], 
						double RootPoly[], 
						gsl_complex EvaluatedRootPoly[]);
void printPolynomials(int badplayers, double polynomials[][CONFIDENCE_PARAM][badplayers], double RootPoly[]);
void GenerateRandomPoly(int badplayers, double polynomials[][CONFIDENCE_PARAM][badplayers], double RootPoly[]);
void printComplexPolynomials(int badplayers, gsl_complex polynomials[][CONFIDENCE_PARAM][badplayers], double RootPoly[]);
int AreComplexEqual(gsl_complex A, gsl_complex B, int precision);

#endif
