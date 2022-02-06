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
		printf(" %+fx^%d", RootPoly[j], j);
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
				printf(" %+fx^%d", polynomials[l][i][j], j);
			}
			printf("\n");
		}
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Prints all Polynomials. gsl_complex version
*/
void printComplexPolynomials(int badplayers, gsl_complex polynomials[][CONFIDENCE_PARAM][badplayers], double RootPoly[])
{
	#ifndef DEBUG
		return;
	#endif

	TraceDebug("%s*enter\n", __FUNCTION__);

	printf("Root polynomial:\n");
	for (int j = 0; j < badplayers; j++)
	{
		printf(" %+fx^%d", RootPoly[j], j);
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
				printf(" %+f%+fx^%d", GSL_REAL(polynomials[l][i][j]), GSL_IMAG(polynomials[l][i][j]), j);
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
						gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM], 
						double RootPoly[], 
						gsl_complex EvaluatedRootPoly[])
{
	TraceDebug("%s*enter\n", __FUNCTION__);
	gsl_complex X;

	for (int i = 0; i < numOfNodes; i++)
	{
		if (i != 0)
			X = gsl_complex_pow_real(RootOfUnity, i);
		else
			X = gsl_complex_pow_real(RootOfUnity, numOfNodes);

		EvaluatedRootPoly[i] = gsl_poly_complex_eval(RootPoly, badplayers, X);
		TraceDebug("node [%d] evaluated with [%f%+fi]\n", i, GSL_REAL(X), GSL_IMAG(X));
		for (int l = 0; l < numOfNodes; l++)
		{
			for (int j = 0; j < CONFIDENCE_PARAM; j++)
			{
				polyEvals[i][l][j] = gsl_poly_complex_eval(polynomials[l][j], badplayers, X);
			}		
		}
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Prints the result from the evaluation.
*/
void printEvaluatedPolys(int numOfNodes, gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM], gsl_complex EvalRootPoly[])
{
	#ifndef DEBUG
		return;
	#endif
	
	TraceDebug("%s*enter\n", __FUNCTION__);

	//Calcuate root polynomial
	for (int i = 0; i < numOfNodes; i++)
	{
		if ((proc_id != i) && !IsDealer)
			continue;

		TraceInfo("node %d\n", i);
		printf("RootPoly: [%f%+fi]\n", GSL_REAL(EvalRootPoly[i]), GSL_IMAG(EvalRootPoly[i]));
		for (int l = 0; l < numOfNodes; l++)
		{
			for (int j = 0; j < CONFIDENCE_PARAM; j++)
			{
				printf("[%f%+fi]\n", GSL_REAL(polyEvals[i][l][j]), GSL_IMAG(polyEvals[i][l][j]));
			}
			printf("\n");
		}
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Custome function to check if two complex numbers are the same.
 * precision can be set, default is 3 decimals.
*/
int AreComplexEqual(gsl_complex A, gsl_complex B, int precision)
{
	double compareWith;

	if (precision <= 0)
		compareWith = 0.0001;
	else
		compareWith = pow(10, -precision);

	if ((fabs(GSL_REAL(A) - GSL_REAL(B)) < compareWith) &&
		(fabs(GSL_IMAG(A) - GSL_IMAG(B)) < compareWith))
		return 1;
	else
		return 0;
}