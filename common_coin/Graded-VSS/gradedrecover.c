#include "gradedrecover.h"
#include <gsl/gsl_spline.h>
#include <gsl/gsl_errno.h>
#include "polyfunc.h"

int ParsePiece(char *Piece, double RootPolynomial[]);
void GetPieces(struct servers reqServer[], double RootPolynomial[]);
double CalculatePolynomial(double EvaluatedRootPoly[]);

void SimpleGradedRecover(struct servers reqServer[], double EvaluatedRootPoly[])
{
	char Piece[StringSecreteSize];
	memset(Piece, 0, StringSecreteSize);
	TraceInfo("%s*enter\n", __FUNCTION__);

	sprintf(Piece, "%d%s%f%s", proc_id, MESSAGE_DELIMITER, EvaluatedRootPoly[proc_id], MESSAGE_DELIMITER);
	Distribute(reqServer, Piece);
	GetPieces(reqServer, EvaluatedRootPoly);

	for (int i = 0; i < numOfNodes; i++)
		printf("i:[%d] Si:[%f]\n", i, EvaluatedRootPoly[i]);

	double finale = CalculatePolynomial(EvaluatedRootPoly);
	TraceInfo("%s*exit*finale[%f]\n", __FUNCTION__, round(finale));
}

/**
 * Receive the pieces of Simple Graded - Recover phase
*/
 void GetPieces(struct servers reqServer[], double EvaluatedRootPoly[])
{
	char recvBuffer[StringSecreteSize + 1];
	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;

		zmq_recv(reqServer[proc_id].value, recvBuffer, StringSecreteSize, 0);
		TraceDebug("Received data as server[%d]: [%s]\n", proc_id, recvBuffer);

		ParsePiece(recvBuffer, EvaluatedRootPoly);
		memset(recvBuffer, 0, sizeof(recvBuffer));
	}

	TraceInfo("%s*exit\n", __FUNCTION__);
}

/**
 * Parse the message received from dealer in step 2.
 */
int ParsePiece(char *Piece, double EvaluatedRootPoly[])
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

	if (outArray[Process].code == 0)
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

double CalculatePolynomial(double EvaluatedRootPoly[])
{
	int size = 0;
	int counter = 0;
	int status;

	for (int i = 0; i < numOfNodes; i++)
		if (outArray[i].code != 0 && (fabs(EvaluatedRootPoly[i]) >= 0.0001))
			size++;

	TraceDebug("%s*size:[%d]\n",__FUNCTION__, size);

	if (size <= 0)
	{
		TraceInfo("I can't do this man...\n");
		return 0;
	}

	double X_1[size], dummy_X[size];
	double Y_1[size], dummy_Y[size];
	double result;

	for (int i = 0; i < numOfNodes; i++)
	{
		if (outArray[i].code > 0  && i == 0)
		{
			X_1[i] = pow(RootOfUnity, numOfNodes);
			Y_1[i] = EvaluatedRootPoly[i];
			counter++;
		}
		else if (outArray[i].code > 0 )
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
		return 0;
	}

	result = gsl_spline_eval(spline, 0, acc);
	gsl_spline_free(spline);
	gsl_interp_accel_free(acc);

	return result;
}
