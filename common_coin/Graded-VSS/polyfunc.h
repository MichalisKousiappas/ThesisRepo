#ifndef POLYFUNC_H
#define POLYFUNC_H

#include "globals.h"
#include <gsl/gsl_poly.h>

void GenerateRandomPoly(int badplayers, double polynomials[][badplayers]);
void printPolynomials(int badplayers, double polynomials[][badplayers]);
void evaluatePolynomials(int badplayers, double polynomials[][badplayers], double polyEvals[], int X);
void printEvaluatedPolys(double polyEvals[]);

#endif
