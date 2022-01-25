#include "dealerfunc.h"
#include "functions.h"

// Local Declarations
void DealerDistributeSecret(struct servers reqServer[], int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int EvaluatedRootPoly[]);
char *BuildSecretString(int node, int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int EvaluatedRootPoly[]);

/**
 * Dealer distribute the polynomial evaluations for each node
*/
char *SimpleGradedShare(struct servers syncServer[], int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int EvaluatedRootPoly[])
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
		result = GetFromDistributor(syncServer, dealer);
		WaitForDealerSignal(syncServer);
	}

	TraceInfo("%s*exit\n", __FUNCTION__);
	return result;
}

/**
 * Builds the Dealers secret to each node
*/
char *BuildSecretString(int node, int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int EvaluatedRootPoly[])
{
	int length = 0;
	char *result = (char*) malloc(StringSecreteSize);
	memset(result, 0, sizeof(StringSecreteSize)-1);

	//Secret starts with the evaluated root polynomial
	length += snprintf(result+length , StringSecreteSize-length, "%d%s", EvaluatedRootPoly[node], MESSAGE_DELIMITER);

	for (int j = 0; j < numOfNodes; j++)
	{
		for(int i = 0; i < CONFIDENCE_PARAM; i++)
		{
			length += snprintf(result+length , StringSecreteSize-length, "%d%s", polyEvals[node][j][i], MESSAGE_DELIMITER);
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
void DealerDistributeSecret(struct servers reqServer[], int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int EvaluatedRootPoly[])
{
	char sendBuffer[StringSecreteSize + 1];
	char recvBuffer[5];
	int requestor;

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);

	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		zmq_recv(reqServer[dealer].value, recvBuffer, 5, 0);
		TraceDebug("Received data as dealer: [%s]\n", recvBuffer);
		requestor = atoi(recvBuffer);

		// Build secret for each node
		sprintf(sendBuffer, "%s", BuildSecretString(requestor, polyEvals, EvaluatedRootPoly));

		zmq_send(reqServer[requestor].value, sendBuffer, StringSecreteSize, 0);
		TraceDebug("Send data as dealer to [%d]: [%s]\n", requestor, sendBuffer);

		memset(recvBuffer, 0, sizeof(recvBuffer));
		messages++;
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}