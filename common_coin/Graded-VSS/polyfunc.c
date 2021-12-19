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
		for (int j = 0; j < badplayers; j++)
		{
			printf(" %.2fx^%d",polynomials[i][j], j);
			if (j != badplayers - 1) 
				printf(" + ");
		}
		printf("\n");
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Calculates the polynomial for the given X
*/
void evaluatePolynomials(int badplayers, double polynomials[][badplayers], double polyEvals[][CONFIDENCE_PARAM])
{
	int X = 1;

	for (int i = 0; i < numOfNodes; i++)
	{
		X = (X+1) % 3;
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
			polyEvals[i][j] = gsl_poly_eval( polynomials[j], badplayers, X);
	}
}

/**
 * Prints the result from the evaluation.
*/
void printEvaluatedPolys(int numOfNodes, double polyEvals[][CONFIDENCE_PARAM])
{
	for (int i = 0; i < numOfNodes; i++)
	{
		TraceInfo("node %d ", i);
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
		{
			printf("[%.2f] ", polyEvals[i][j]);
		}
		printf("\n");
	}
}