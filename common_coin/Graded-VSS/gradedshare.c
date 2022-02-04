#include "gradedshare.h"
#include "gradeddecide.h"

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