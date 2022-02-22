#include "polyfunc.h"

/**
 * Generate Random Polynomials.
 * The number of polynomials is always the confidence param
 * and the degree is the number of bad players
*/
void GenerateRandomPoly(int badplayers, double polynomials[][CONFIDENCE_PARAM][badplayers], double RootPoly[])
{
	TraceDebug("%s*enter\n", __FUNCTION__);

	srand(time(0));

	//Calcuate root polynomial
	for (int i=0; i< badPlayers; i++)
		RootPoly[i] = rand() % MAX_COEFICIENT;

	//Calculate the rest of the polynomials
	for (int l=0; l< numOfNodes; l++)
	{
		for (int i = 0; i < CONFIDENCE_PARAM; i++)
		{
			for (int j = 0; j < badPlayers; j++)
			{
				polynomials[l][i][j] = rand() % MAX_COEFICIENT;
			}
		}
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Prints all Polynomials.
*/
void printPolynomials(int badplayers, double polynomials[][CONFIDENCE_PARAM][badplayers], double RootPoly[])
{
	#ifndef DEBUG
		return;
	#endif

	TraceDebug("%s*enter\n", __FUNCTION__);

	printf("Root polynomial:\n");
	for (int j = 0; j < badplayers; j++)
	{
		printf(" %fx^%d", RootPoly[j], j);
		if (j != badplayers - 1) 
			printf(" + ");
	}
	printf("\n");

	for (int l=0; l< numOfNodes; l++)
	{
		TraceDebug("node %d has:\n", l);
		for (int i = 0; i < CONFIDENCE_PARAM; i++)
		{
			printf("\t\tpolynomial %d is:", i);
			for (int j = 0; j < badplayers; j++)
			{
				printf(" %fx^%d", polynomials[l][i][j], j);
				if (j != badplayers - 1) 
					printf(" + ");
			}
			printf("\n");
		}
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Calculates the polynomial for the given X
*/
void evaluatePolynomials(int badplayers, 
						double polynomials[][CONFIDENCE_PARAM][badplayers],
						double polyEvals[][numOfNodes][CONFIDENCE_PARAM], 
						double RootPoly[], 
						double EvaluatedRootPoly[])
{
	TraceDebug("%s*enter\n", __FUNCTION__);
	double X;

	for (int i = 0; i < numOfNodes; i++)
	{
		if (i != 0)
			X = pow(RootOfUnity, i);
		else
			X = pow(RootOfUnity, numOfNodes);

		EvaluatedRootPoly[i] = gsl_poly_eval(RootPoly, badplayers, X);
		TraceDebug("node [%d] evaluated with [%f]\n", i, X);
		for (int l = 0; l < numOfNodes; l++)
		{
			for (int j = 0; j < CONFIDENCE_PARAM; j++)
			{
				polyEvals[i][l][j] = gsl_poly_eval(polynomials[l][j], badplayers, X);
			}		
		}
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Prints the result from the evaluation.
*/
void printEvaluatedPolys(int numOfNodes, double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double EvalRootPoly[])
{
	#ifndef DEBUG
		return;
	#endif
	
	TraceDebug("%s*enter\n", __FUNCTION__);

	//Calcuate root polynomial
	for (int i = 0; i < numOfNodes; i++)
	{
		if ((proc_id != i) && (proc_id != dealer))
			continue;

		TraceInfo("node %d\n", i);
		printf("RootPoly: [%f]\n", EvalRootPoly[i]);
		for (int l = 0; l < numOfNodes; l++)
		{
			for (int j = 0; j < CONFIDENCE_PARAM; j++)
			{
				printf("[%f] ", polyEvals[i][l][j]);
			}
			printf("\n");
		}
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * printRootPolyOnly
 * print the root polynomial only when debugging traces are off.
*/
void printRootPolyOnly(double RootPoly[])
{
	#ifdef DEBUG
		return;
	#endif

	printf("Root polynomial:\n");
	for (int j = 0; j < badPlayers; j++)
	{
		printf(" %fx^%d", RootPoly[j], j);
		if (j != badPlayers - 1) 
			printf(" + ");
	}
	printf("\n");
}