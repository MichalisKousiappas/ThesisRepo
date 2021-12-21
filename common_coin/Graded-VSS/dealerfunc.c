#include "dealerfunc.h"
#include "functions.h"

// Local Declarations
void DealerDistributeSecret(struct servers reqServer[], double polyEvals[][CONFIDENCE_PARAM]);
char *BuildSecretString(double polyEvals[]);

/**
 * Dealer distribute the polynomial evaluations for each node
*/
char *DealerDistribute(struct servers syncServer[], double polyEvals[][CONFIDENCE_PARAM])
{
	char *result = {0};

	TraceInfo("%s*enter\n", __FUNCTION__);

	if (proc_id == dealer)
	{
		DealerDistributeSecret(syncServer, polyEvals);
		
		// Dealer process builds its own secret instead of sending it to itself
		result = BuildSecretString(polyEvals[dealer]);
		TraceInfo("%s*distirbutor:[%d] finished. Sending OK signal\n", __FUNCTION__, dealer);
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
char *BuildSecretString(double polyEvals[])
{
	int length = 0;
	char *result = (char*) malloc(SECRETE_SIZE);
	memset(result, 0, sizeof(SECRETE_SIZE));	
	
	for(int i = 0; i < CONFIDENCE_PARAM; i++)
	{
		if (i < CONFIDENCE_PARAM - 1)
			length += snprintf(result+length , SECRETE_SIZE-length, "%0.2f,", polyEvals[i]);  
		else
			length += snprintf(result+length , SECRETE_SIZE-length, "%0.2f", polyEvals[i]);  
	}

	return result;
}

/**
  Send the same message to all other nodes
 */
void DealerDistributeSecret(struct servers reqServer[], double polyEvals[][CONFIDENCE_PARAM])
{
	char sendBuffer [SECRETE_SIZE];
	char recvBuffer [5];
	int requestor;

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);

	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		zmq_recv(reqServer[dealer].value, recvBuffer, 5, 0);
		TraceInfo("Received data as dealer: [%s]\n", recvBuffer);
		requestor = atoi(recvBuffer);

		// Build secret for each node
		sprintf(sendBuffer, "%s", BuildSecretString(polyEvals[requestor]));

		zmq_send(reqServer[requestor].value, sendBuffer, SECRETE_SIZE, 0);
		TraceInfo("Send data as dealer to [%d]: [%s]\n", requestor, sendBuffer);

		memset(recvBuffer, 0, sizeof(recvBuffer));
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}