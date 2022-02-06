#include "gradedshare.h"

// Local Declarations
void DealerDistributeSecret(struct servers reqServer[], gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM], gsl_complex EvaluatedRootPoly[]);
char *BuildSecretString(int node, gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM], gsl_complex EvaluatedRootPoly[]);
char *GetFromDealer(struct servers reqServer[]);

/**
 * Dealer distribute the polynomial evaluations for each node
*/
char *SimpleGradedShare(struct servers syncServer[], gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM], gsl_complex EvaluatedRootPoly[])
{
	char *result = NULL;

	TraceInfo("%s*enter\n", __FUNCTION__);

	if (IsDealer)
	{
		DealerDistributeSecret(syncServer, polyEvals, EvaluatedRootPoly);

		// Dealer process builds its own secret instead of sending it to itself
		result = BuildSecretString(dealer, polyEvals, EvaluatedRootPoly);
		TraceDebug("%s*distirbutor:[%d] finished. Sending OK signal\n", __FUNCTION__, dealer);
		Distribute(syncServer, "OK");
	}
	else
	{
		result = GetFromDealer(syncServer);
		WaitForDealerSignal(syncServer);
	}

	TraceInfo("%s*exit\n", __FUNCTION__);
	return result;
}

/**
 * Builds the Dealers secret to each node
*/
char *BuildSecretString(int node, gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM], gsl_complex EvaluatedRootPoly[])
{
	int length = 0;
	char *result = (char*) malloc(StringSecreteSize);
	memset(result, 0, StringSecreteSize);

	//Secret starts with the evaluated root polynomial
	length += snprintf(result+length , StringSecreteSize-length, "%f%s%f%s", 
						GSL_REAL(EvaluatedRootPoly[node]), 
						COMPLEX_DELIMITER, 
						GSL_IMAG(EvaluatedRootPoly[node]), 
						MESSAGE_DELIMITER);

	for (int j = 0; j < numOfNodes; j++)
	{
		for(int i = 0; i < CONFIDENCE_PARAM; i++)
		{
			length += snprintf(result+length , StringSecreteSize-length, "%f%s%f%s", 
								GSL_REAL(polyEvals[node][j][i]),
								COMPLEX_DELIMITER,
								GSL_IMAG(polyEvals[node][j][i]),
								MESSAGE_DELIMITER);
		}
	}

	//Close the close so parsing can be done correctly
	length += snprintf(result+length , StringSecreteSize-length, "%s", MESSAGE_DELIMITER);

	result[length-1] = '\0';
	return result;
}

/**
  Send the same message to all other nodes
 */
void DealerDistributeSecret(struct servers reqServer[], gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM], gsl_complex EvaluatedRootPoly[])
{
	char sendBuffer[StringSecreteSize + 1];
	char recvBuffer[56];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);

	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;

		// Build secret for each node
		sprintf(sendBuffer, "%s", BuildSecretString(i, polyEvals, EvaluatedRootPoly));
		
		zmq_send(reqServer[i].value, sendBuffer, StringSecreteSize, 0);
		TraceDebug("Send data as dealer to [%d]: [%s]\n", i, sendBuffer);

		zmq_recv(reqServer[dealer].value, recvBuffer, sizeof(recvBuffer) - 1, 0);
		TraceDebug("Received data as dealer: [%s]\n", recvBuffer);

		memset(recvBuffer, 0, sizeof(recvBuffer));
		messages++;
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}

/**
 * Get distributors secret 
 * distributor is the node that initiates Grade-Cast or the dealer
*/
char *GetFromDealer(struct servers reqServer[])
{
	TraceInfo("%s*enter\n", __FUNCTION__);
	char *result = (char*) malloc(StringSecreteSize + 1);
	char sendBuffer[56];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(result, 0, sizeof(StringSecreteSize));

	sprintf(sendBuffer, "%d %s", proc_id, "OK");

	zmq_recv(reqServer[proc_id].value, result, StringSecreteSize, 0);
	TraceDebug("Received secret from dealer: [%s]\n", result);

	TraceDebug("Sending data as client[%d] to dealer: [%s]\n", proc_id, sendBuffer);
	zmq_send(reqServer[dealer].value, sendBuffer, strlen(sendBuffer), 0);

	TraceInfo("%s*exit\n", __FUNCTION__);
	return result;
}

/**
 * Parse the secret received from dealer.
 */
int ParseSecret(char *secret, gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM], gsl_complex EvaluatedRootPoly[])
{
	// Dealer process does not need to parse the secret
	if (IsDealer)
		return 0;

	TraceInfo("%s*enter\n", __FUNCTION__);

	if (secret[strlen(secret) - 1] != '|')
	{
		TraceInfo("%s*exit*Invalid Secret\n", __FUNCTION__);
		return 1;
	}

	char* token = strtok(secret, ALL_MESSAGE_DELIMITERS);
	GSL_REAL(EvaluatedRootPoly[proc_id]) = strtod(token, NULL);
	token = strtok(0, ALL_MESSAGE_DELIMITERS);
	GSL_IMAG(EvaluatedRootPoly[proc_id]) = strtod(token, NULL);

	for (int i = 0; i < numOfNodes; i++)
	{
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
		{
			for(int d = 0; d < 2; d++)
			{
				token = strtok(0, ALL_MESSAGE_DELIMITERS);
				if (token != NULL && d == 0)
					GSL_REAL(polyEvals[proc_id][i][j]) = strtod(token, NULL);
				else if (token != NULL && d == 1)
					GSL_IMAG(polyEvals[proc_id][i][j]) = strtod(token, NULL);
				else
				{
					TraceInfo("%s*exit*Invalid Secret[%d][%d]\n", __FUNCTION__,i, j);
					return 1;
				}
			}
		}
	}

	// When debugging is on, printf the parsed message
	printEvaluatedPolys(numOfNodes, polyEvals, EvaluatedRootPoly);
	TraceInfo("%s*exit\n", __FUNCTION__);
	return 0;
}
