#include "polyfunc.h"

int poly_eval(const int c[], const int len, const double x);

/**
 * Generate Random Polynomials.
 * The number of polynomials is always the confidence param
 * and the degree is the number of bad players
*/
void GenerateRandomPoly(int badplayers, int polynomials[][CONFIDENCE_PARAM][badplayers], int RootPoly[])
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
void printPolynomials(int badplayers, int polynomials[][CONFIDENCE_PARAM][badplayers], int RootPoly[])
{
	#ifndef DEBUG
		return;
	#endif

	TraceDebug("%s*enter\n", __FUNCTION__);

	printf("Root polynomial:\n");
	for (int j = 0; j < badplayers; j++)
	{
		printf(" %dx^%d",RootPoly[j], j);
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
				printf(" %dx^%d",polynomials[l][i][j], j);
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
void evaluatePolynomials(int badplayers, int polynomials[][CONFIDENCE_PARAM][badplayers], int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int RootPoly[], int EvaluatedRootPoly[])
{
	TraceDebug("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < numOfNodes; i++)
	{
		int X = (int) pow(RootOfUnity, i);
		EvaluatedRootPoly[i] = poly_eval(RootPoly, badplayers, X);
		TraceDebug("node [%d] evaluated with [%d]\n", i, X);
		for (int l = 0; l < numOfNodes; l++)
		{
			for (int j = 0; j < CONFIDENCE_PARAM; j++)
			{
				polyEvals[i][l][j] = poly_eval(polynomials[l][j], badplayers, X);
			}		
		}
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Prints the result from the evaluation.
*/
void printEvaluatedPolys(int numOfNodes, int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int EvalRootPoly[])
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
		printf("RootPoly: [%d]\n", EvalRootPoly[i]);
		for (int l = 0; l < numOfNodes; l++)
		{
			for (int j = 0; j < CONFIDENCE_PARAM; j++)
			{
				printf("[%d] ", polyEvals[i][l][j]);
			}
			printf("\n");
		}
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

int poly_eval(const int c[], const int len, const double x)
{
  int i;
  int ans = c[len-1];
  for(i=len-1; i>0; i--) ans = c[i-1] + x * ans;
  return ans;
}