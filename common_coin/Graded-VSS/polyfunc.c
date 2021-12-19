#include "polyfunc.h"

/**
 * Generate Random Polynomials.
 * The number of polynomials is always the confidence param
 * and the degree is the number of bad players
*/
void GenerateRandomPoly(int badplayers, double polynomials[][badplayers])
{
	TraceDebug("%s*enter\n", __FUNCTION__);

	srand(time(0));

	for (int i = 0; i < CONFIDENCE_PARAM; i++)
	{
		for (int j = 0; j < badPlayers; j++)
		{
			polynomials[i][j] = rand() % 100;
		}
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Prints all Polynomials.
*/
void printPolynomials(int badplayers, double polynomials[][badplayers])
{
	TraceDebug("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < CONFIDENCE_PARAM; i++)
	{
		TraceDebug("polynomial %d is:", i);
		for (int j = 0; j < badPlayers; j++)
		{
			printf(" %.2fx^%d",polynomials[i][j], j);
			if (j != badPlayers - 1) 
				printf(" + ");
		}
		printf("\n");
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Calculates the polynomial for the given X
*/
void evaluatePolynomials(int badplayers, double polynomials[][badplayers], double polyEvals[], int X)
{
	TraceDebug("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < CONFIDENCE_PARAM; i++)
		polyEvals[i] = gsl_poly_eval( polynomials[i], badPlayers, X);

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Prints the result from the evaluation.
*/
void printEvaluatedPolys(double polyEvals[])
{
	for (int i = 0; i < CONFIDENCE_PARAM; i++)
	{
		TraceInfo("poly %d evaluation is: [%.2f]\n", i, polyEvals[i]);
	}
}