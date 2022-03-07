#include "gradedrecover.h"
#include <gsl/gsl_spline.h>
#include <gsl/gsl_errno.h>
#include "polyfunc.h"
#include <gsl/gsl_fft_complex.h>

#define SET_REAL(z,i) ((z)[2*(i)])
#define SET_IMAG(z,i) ((z)[2*(i)+1])

int ParsePiece(char *Piece, gsl_complex RootPolynomial[]);
void GetPieces(struct servers reqServer[], gsl_complex RootPolynomial[]);
double CalculatePolynomial(gsl_complex EvaluatedRootPoly[]);
void printTables(int size, gsl_complex X_1[], gsl_complex Y_1[]);

int GetNextPowerOfTwo(int x)
{
    if (!((x != 0) && ((x & (x - 1)) == 0)))
	{
		int power = 1;
		while(power < x)
			power*=2;
	
		return power;
	}
	return x;
}

void SimpleGradedRecover(struct servers reqServer[], gsl_complex EvaluatedRootPoly[])
{
	char Piece[StringSecreteSize];
	memset(Piece, 0, StringSecreteSize);
	TraceInfo("%s*enter\n", __FUNCTION__);

	sprintf(Piece, "%d%s%f%s%+f%s", proc_id, 
			MESSAGE_DELIMITER, 
			GSL_REAL(EvaluatedRootPoly[proc_id]),
			COMPLEX_DELIMITER,
			GSL_IMAG(EvaluatedRootPoly[proc_id]),
			MESSAGE_DELIMITER);

	Distribute(reqServer, Piece);
	GetPieces(reqServer, EvaluatedRootPoly);

	for (int i = 0; i < numOfNodes; i++)
		printf("i:[%d] Si:[%f%+fi]\n", i, GSL_REAL(EvaluatedRootPoly[i]), GSL_IMAG(EvaluatedRootPoly[i]));

	double finale = CalculatePolynomial(EvaluatedRootPoly);
	TraceInfo("%s*exit*finale[%f]\n", __FUNCTION__, finale);
}

/**
 * Receive the pieces of Simple Graded - Recover phase
*/
 void GetPieces(struct servers reqServer[], gsl_complex EvaluatedRootPoly[])
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
int ParsePiece(char *Piece, gsl_complex EvaluatedRootPoly[])
{
	TraceInfo("%s*enter\n", __FUNCTION__);

	if (Piece[strlen(Piece) - 1] != '|')
	{
		TraceInfo("%s*exit*Invalid Piece 1\n", __FUNCTION__);
		return 1;
	}

	char* token = strtok(Piece, ALL_MESSAGE_DELIMITERS);
	TraceDebug("%s*token[%d]\n", __FUNCTION__, atoi(token));
	int Process = atoi(token);

	if (outArray[Process].code == 0)
	{
		TraceInfo("%s*exit*ignoring piece\n", __FUNCTION__);
		return 1;
	}

	for(int d = 0; d < 2; d++)
	{
		token = strtok(0, ALL_MESSAGE_DELIMITERS);
		if (token != NULL && d == 0)
			GSL_REAL(EvaluatedRootPoly[Process]) = strtod(token, NULL);
		else if (token != NULL && d == 1)
			GSL_IMAG(EvaluatedRootPoly[Process]) = strtod(token, NULL);
		else
		{
			TraceInfo("%s*exit*Invalid Piece 2\n", __FUNCTION__);
			return 1;
		}
	}

	TraceInfo("%s*exit\n", __FUNCTION__);
	return 0;
}

void printTables(int size, gsl_complex X_1[], gsl_complex Y_1[])
{
	#ifndef DEBUG
		return;
	#endif

	printf("\n");
	for (int i = 0; i < size; i++)
		printf("Xi[%f%+fi] ", GSL_REAL(X_1[i]), GSL_IMAG(X_1[i]));
	printf("\n");
	for (int i = 0; i < size; i++)
		printf("Xi[%f%+fi] ", GSL_REAL(Y_1[i]), GSL_IMAG(Y_1[i]));
	printf("\n");
}

double CalculatePolynomial(gsl_complex EvaluatedRootPoly[])
{
	int size = 0;
	int counter = 0;
	int status;
	int FFTsize = 0;

	//GSL_SET_COMPLEX(&result, 0, 0);

	for (int i = 0; i < numOfNodes; i++)
		if (outArray[i].code != 0)
			size++;

	TraceDebug("%s*size:[%d]\n",__FUNCTION__, size);

	if (size < (numOfNodes - badPlayers))
	{
		TraceInfo("I can't do this man...\n");
		return 0;
	}

	FFTsize = GetNextPowerOfTwo(size);
	double FFTData[FFTsize*2];
	memset(FFTData, 0, FFTsize*2);

	for (int i = 0; i < numOfNodes; i++)
	{
		if (outArray[i].code > 0 )
		{
			SET_REAL(FFTData, i) = GSL_REAL(EvaluatedRootPoly[i]);
			SET_IMAG(FFTData, i) = GSL_IMAG(EvaluatedRootPoly[i]);
			counter++;
		}
	}

	//parallel_array_merge_sort(0, size - 1, X_1, dummy_X, Y_1, dummy_Y);
	//printTables(size,X_1,Y_1);

	// Disable gsl errors
	gsl_set_error_handler_off();
/*
    gsl_interp_accel *acc = gsl_interp_accel_alloc();
    gsl_spline *spline = gsl_spline_alloc(gsl_interp_steffen, size);

    status = gsl_spline_init(spline, X_1, Y_1, size);

	if (status)
	{
		TraceInfo("Interpolation cannot be performed. ErrorCode[%d] Description:[%s]\n", status, gsl_strerror(status));
		GSL_SET_COMPLEX(&result, 0, 0);
		return result;
	}

	result = gsl_spline_eval(spline, 0, acc);
	gsl_spline_free(spline);
	gsl_interp_accel_free(acc);

	return result;
	*/

	status = gsl_fft_complex_radix2_inverse(FFTData, 1, FFTsize);
	if (status)
	{
		TraceInfo("inverse FFT cannot be performed. ErrorCode[%d] Description:[%s]\n", status, gsl_strerror(status));
		return 0;
	}

	double ha = 0;
	gsl_complex hehe;
	for (int i = 0; i < size; i++)
	{
		printf("[%f] ", FFTData[i]);
		ha += FFTData[i];
	}
	printf("\nha:[%f]\n", ha);
	hehe= gsl_poly_complex_eval(FFTData, badPlayers, gsl_complex_pow_real(RootOfUnity[proc_id], proc_id != 0 ? proc_id : numOfNodes));
	printf("\nhehe:[%f%+fi]\n", GSL_REAL(hehe), GSL_IMAG(hehe));

	return FFTData[0];
}
