#include "gradedshare.h"

// Local Declarations
void DealerDistributeSecret(struct servers reqServer[], double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double EvaluatedRootPoly[]);
char *BuildSecretString(int node, double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double EvaluatedRootPoly[]);
char *GetFromDealer(struct servers reqServer[]);

/**
 * Dealer distribute the polynomial evaluations for each node
*/
char *SimpleGradedShare(struct servers syncServer[], double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double EvaluatedRootPoly[])
{
	char *result = NULL;
	printf("-------------------SimpleGraded Share----------------------------\n");
	TraceDebug("%s*enter\n", __FUNCTION__);

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

	TraceDebug("%s*exit\n", __FUNCTION__);
	return result;
}

/**
 * Builds the Dealers secret to each node
*/
char *BuildSecretString(int node, double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double EvaluatedRootPoly[])
{
	int length = 0;
	char *result = (char*) malloc(StringSecreteSize);
	memset(result, 0, StringSecreteSize);

	//Secret starts with the evaluated root polynomial
	length += snprintf(result+length , StringSecreteSize-length, "%f%s", EvaluatedRootPoly[node], MESSAGE_DELIMITER);

	for (int j = 0; j < numOfNodes; j++)
	{
		for(int i = 0; i < CONFIDENCE_PARAM; i++)
		{
			length += snprintf(result+length , StringSecreteSize-length, "%f%s", polyEvals[node][j][i], MESSAGE_DELIMITER);
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
void DealerDistributeSecret(struct servers reqServer[], double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double EvaluatedRootPoly[])
{
	char sendBuffer[StringSecreteSize + 1];
	char recvBuffer[56];
	int oldTimeoutValue = TIMEOUT_MULTIPLIER * numOfNodes;
	int newTimeoutValue = TIMEOUT_MULTIPLIER;

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceDebug("%s*enter\n", __FUNCTION__);

	zmq_setsockopt(reqServer[proc_id].value, ZMQ_RCVTIMEO, &newTimeoutValue, sizeof(int));

	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if ((i == proc_id) || TimedOut[i] == 1)
			continue;

		// Build secret for each node
		sprintf(sendBuffer, "%s", BuildSecretString(i, polyEvals, EvaluatedRootPoly));
		
		zmq_send(reqServer[i].value, sendBuffer, StringSecreteSize, 1);
		TraceDebug("Send data as dealer to [%d]: [%s]\n", i, sendBuffer);

		if (zmq_recv(reqServer[dealer].value, recvBuffer, sizeof(recvBuffer) - 1, 0) == -1)
			TraceInfo("No response from: [%d]\n", i);
		else
			TraceDebug("Received data as dealer: [%s]\n", recvBuffer);

		memset(recvBuffer, 0, sizeof(recvBuffer));
		messages++;
	}

	zmq_setsockopt(reqServer[proc_id].value, ZMQ_RCVTIMEO, &oldTimeoutValue, sizeof(int));
	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Get distributors secret 
 * distributor is the node that initiates Grade-Cast or the dealer
*/
char *GetFromDealer(struct servers reqServer[])
{
	TraceDebug("%s*enter\n", __FUNCTION__);
	char *result = (char*) malloc(StringSecreteSize + 1);
	char sendBuffer[56];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(result, 0, sizeof(StringSecreteSize));

	sprintf(sendBuffer, "%d %s", proc_id, "OK");

	if (zmq_recv(reqServer[proc_id].value, result, StringSecreteSize, 0) == -1)
	{
		TimeoutDetected(__FUNCTION__, dealer);
		memset(result, 0, sizeof(StringSecreteSize));
		return result;
	}
	else
		TraceDebug("Received secret from dealer: [%s]\n", result);

	TraceDebug("Sending data as client[%d] to dealer: [%s]\n", proc_id, sendBuffer);
	zmq_send(reqServer[dealer].value, sendBuffer, strlen(sendBuffer), 0);

	TraceDebug("%s*exit\n", __FUNCTION__);
	return result;
}

/**
 * Parse the secret received from dealer.
 */
int ParseSecret(char *secret, double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double EvaluatedRootPoly[])
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

	char* token = strtok(secret, MESSAGE_DELIMITER);
	EvaluatedRootPoly[proc_id] = strtod(token, NULL);

	for (int i = 0; i < numOfNodes; i++)
	{
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
		{
			token = strtok(0, MESSAGE_DELIMITER);
			if (token != NULL)
				polyEvals[proc_id][i][j] = strtod(token, NULL);
			else
			{
				TraceInfo("%s*exit*Invalid Secret[%d][%d]\n", __FUNCTION__,i, j);
				return 1;
			}
		}
	}

	// When debugging is on, printf the parsed message
	printEvaluatedPolys(numOfNodes, polyEvals, EvaluatedRootPoly);
	TraceInfo("%s*exit\n", __FUNCTION__);
	return 0;
}
