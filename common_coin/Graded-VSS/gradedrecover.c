#include "gradedrecover.h"
#include <gsl/gsl_spline.h>
#include <gsl/gsl_errno.h>
#include "polyfunc.h"

// Local Function declarion
int ParsePiece(char *Piece, double RootPolynomial[], struct output candidate[]);
int CalculatePolynomial(double EvaluatedRootPoly[], struct output candidate[], double *finale);
void BuildPiece(int node, double EvaluatedRootPoly[], char result[]);

/**
 * SimpleGradedRecover
*/
void SimpleGradedRecover(struct servers reqServer[], 
						double EvaluatedRootPoly[],
						struct output candidate[],
						int tally[])
{
	int flag = 0;
	double finale = 0;
	char GradedCastMessage[StringSecreteSize];
	char BuildedPiece[StringSecreteSize];

	memset(BuildedPiece, 0, sizeof(BuildedPiece));
	memset(GradedCastMessage, 0, sizeof(GradedCastMessage));

	printf("-------------------SimpleGraded Recover----------------------------\n");
	TraceInfo("%s*enter\n", __FUNCTION__);

	// all processes take turn and distribute their "secret"
	for (int distributor = 0; distributor < numOfNodes; distributor++)
	{
		BuildPiece(distributor, EvaluatedRootPoly, BuildedPiece);
		GradeCastPhaseA(reqServer, distributor, BuildedPiece, GradedCastMessage);

		if (candidate[distributor].code > 0)
		{
			// Parse Piece, if message seems invalid then reject the sender
			if (ParsePiece(GradedCastMessage, EvaluatedRootPoly, candidate))
			{
				candidate[distributor].code = 0;
				candidate[distributor].value = 0;
			}
		}
		printf("----------------------------------------\n");
		memset(BuildedPiece, 0, sizeof(BuildedPiece));
		memset(GradedCastMessage, 0, sizeof(GradedCastMessage));
	}

	#ifdef DEBUG
		for (int i = 0; i < numOfNodes; i++)
			printf("i:[%d] Si:[%f]\n", i, EvaluatedRootPoly[i]);
	#endif

	int status = CalculatePolynomial(EvaluatedRootPoly, candidate, &finale);

	if (status)
		return;

	TraceInfo("%s*finale[%f]\n", __FUNCTION__, round(finale));

	for (int i = 0; i < numOfNodes; i++)
	{
		if (candidate[i].code == 0)
		{
			tally[i] = 0;
			continue;
		}
		
		tally[i] = ((int) round(finale) % maxNumberOfMessages > 0) ? 1 : 0;

		if (tally[i] == 0)
			flag = 1;
	}

	if (flag)
		memset(tally, 0, numOfNodes);

	TraceInfo("%s*exit\n", __FUNCTION__);
	printf("----------------------------------------\n");
}

/**
 * Parse the message received from dealer in step 2.
 */
int ParsePiece(char *Piece, double EvaluatedRootPoly[], struct output candidate[])
{
	TraceInfo("%s*enter\n", __FUNCTION__);

	if (Piece[strlen(Piece) - 1] != '|')
	{
		TraceInfo("%s*exit*Invalid Piece 1\n", __FUNCTION__);
		return 1;
	}

	char* token = strtok(Piece, MESSAGE_DELIMITER);
	TraceDebug("%s*token[%d]\n", __FUNCTION__, atoi(token));
	int Process = atoi(token);

	if (candidate[Process].code == 0)
	{
		TraceInfo("%s*exit*ignoring piece\n", __FUNCTION__);
		return 1;
	}

	token = strtok(0, MESSAGE_DELIMITER);
	if (token != NULL)
		EvaluatedRootPoly[Process] = strtod(token, NULL);
	else
	{
		TraceInfo("%s*exit*Invalid Piece 2\n", __FUNCTION__);
		return 1;
	}

	TraceInfo("%s*exit\n", __FUNCTION__);
	return 0;
}

/**
 * Print the 2 given tables. Both tables must have the same size
 */
void printTables(int size, double X_1[], double Y_1[])
{
	#ifndef DEBUG
		return;
	#endif

	printf("\n");
	for (int i = 0; i < size; i++)
		printf("Xi[%f] ", X_1[i]);
	printf("\n");
	for (int i = 0; i < size; i++)
		printf("Yi[%f] ", Y_1[i]);
	printf("\n");
}

/**
 * Performs the interpolation that will then give the true secret from F(0)
 */
int CalculatePolynomial(double EvaluatedRootPoly[], struct output candidate[], double *finale)
{
	int size = 0;
	int counter = 0;
	int status;

	for (int i = 0; i < numOfNodes; i++)
		if (candidate[i].code != 0 && (fabs(EvaluatedRootPoly[i]) >= 0.0001))
			size++;

	TraceDebug("%s*size:[%d]\n",__FUNCTION__, size);

	if (size <= 0)
	{
		TraceInfo("I can't do this man...\n");
		return 1;
	}

	double X_1[size], dummy_X[size];
	double Y_1[size], dummy_Y[size];
	double result;

	for (int i = 0; i < numOfNodes; i++)
	{
		if (candidate[i].code > 0  && i == 0)
		{
			X_1[i] = pow(RootOfUnity, numOfNodes);
			Y_1[i] = EvaluatedRootPoly[i];
			counter++;
		}
		else if (candidate[i].code > 0 )
		{
			X_1[counter] = pow(RootOfUnity, i);
			Y_1[counter] = EvaluatedRootPoly[i];
			counter++;
		}
	}

	parallel_array_merge_sort(0, size - 1, X_1, dummy_X, Y_1, dummy_Y);
	printTables(size,X_1,Y_1);

	// Disable gsl errors
	gsl_set_error_handler_off();

    gsl_interp_accel *acc = gsl_interp_accel_alloc();
    gsl_spline *spline = gsl_spline_alloc(gsl_interp_steffen, size);

    status = gsl_spline_init(spline, X_1, Y_1, size);

	if (status)
	{
		TraceInfo("Interpolation cannot be performed. ErrorCode[%d] Description:[%s]\n", status, gsl_strerror(status));
		return 2;
	}

	result = gsl_spline_eval(spline, 0, acc);
	gsl_spline_free(spline);
	gsl_interp_accel_free(acc);

	*finale = result;
	return 0;
}

/**
 * Build Piece that you will share.
 */
void BuildPiece(int node, double EvaluatedRootPoly[], char result[])
{
	TraceDebug("%s*enter\n", __FUNCTION__);
	int length = 0;

	if (node != proc_id)
	{
		TraceDebug("%s*exit*not my turn yet\n", __FUNCTION__);
		return;
	}

	length +=sprintf(result, "%d%s%f%s", proc_id, MESSAGE_DELIMITER, EvaluatedRootPoly[proc_id], MESSAGE_DELIMITER);

	Traitor(result);

	result[length] = '\0';

	TraceDebug("%s*exit[%d]\n", __FUNCTION__, length);
}