#include "gradecast.h"

void DistributorDistribute(struct servers reqServer[], const char *secret, int distributor);
char *GetFromDistributor(struct servers reqServer[], int distributor);

/*
   Graded-Cast tally validation
 */
struct output ValidateTally(int tally)
{
	TraceInfo("%s*tally:[%d]\n", __FUNCTION__, tally);
	struct output out;

	if (tally >= (2*badPlayers + 1))
	{
		out.value = tally;
		out.code = 2;
	}
	else if ((tally <= (2*badPlayers)) && (tally > badPlayers))
	{
		out.value = tally;
		out.code = 1;
	}
	else
	{
		out.value = 0;
		out.code = 0;
	}

	return out;
}

/**
 * Get distributors secret
 * distributor is the node that initiates Grade-Cast or the dealer
*/
char *GetFromDistributor(struct servers reqServer[], int distributor)
{
	TraceInfo("%s*enter\n", __FUNCTION__);
	char *result = (char*) malloc(StringSecreteSize + 1);
	char sendBuffer[56];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(result, 0, sizeof(StringSecreteSize));

	sprintf(sendBuffer, "%d %s", proc_id, "OK");

	zmq_recv(reqServer[proc_id].value, result, StringSecreteSize, 0);
	TraceDebug("Received secret from distributor: [%s]\n", result);

	TraceDebug("Sending data as client[%d] to distributor[%d]: [%s]\n", proc_id, distributor, sendBuffer);
	zmq_send(reqServer[distributor].value, sendBuffer, strlen(sendBuffer), 0);

	TraceInfo("%s*exit\n", __FUNCTION__);
	return result;
}

/**
  Send the same message to all other nodes
 */
void DistributorDistribute(struct servers reqServer[], const char *secret, int distributor)
{
	char sendBuffer[StringSecreteSize];
	char recvBuffer[56];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);
	// "Secret" binary string
	sprintf(sendBuffer, "%s", secret);

	messages++;
	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;

		TraceDebug("Sending data as distributor to [%d]: [%s]\n", i, sendBuffer);
		zmq_send(reqServer[i].value, sendBuffer, StringSecreteSize, 0);

		zmq_recv(reqServer[distributor].value, recvBuffer, sizeof(recvBuffer) - 1, 0);
		TraceDebug("Received confirmation as distributor[%d]: [%s]\n", distributor, recvBuffer);

		memset(recvBuffer, 0, sizeof(recvBuffer));
		messages++;
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}

/**
 * Grade-Cast Phase A
 * In phase A, the distributor will send his message to all other processes.
*/
char *GradeCastPhaseA(struct servers reqServer[], int distributor, const char *message)
{
	char *result = (char*) malloc(StringSecreteSize);
	char *commonString = {0};

	memset(result, 0, StringSecreteSize);
	TraceInfo("%s*enter\n", __FUNCTION__);

	if (proc_id == distributor)
	{
		DistributorDistribute(reqServer, message, distributor);
		TraceDebug("%s*distirbutor:[%d] finished. Sending OK signal\n", __FUNCTION__, distributor);
		memcpy(result, message, StringSecreteSize);
		commonString = result;
		Distribute(reqServer, "OK");
	}
	else
	{
		commonString = GetFromDistributor(reqServer, distributor);
		WaitForDealerSignal(reqServer);
	}

	TraceInfo("%s*exit\n", __FUNCTION__);
	memcpy(result, commonString, strlen(commonString));
	return result;
}

/**
 * CountSameMessage
 * All processes take turn and send their message to all other nodes.
 * if its not their turn to send they wait for the distributor processes to send.
*/
int CountSameMessage(struct servers reqServer[], const char *message)
{
	int messagesCount = 1;
	char *StringZ;

	TraceInfo("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < numOfNodes; i++)
	{
		if (proc_id == i)
		{
			DistributorDistribute(reqServer, message, i);
			TraceDebug("%s*Process:[%d] finished. Sending OK signal\n", __FUNCTION__, i);
			Distribute(reqServer, "OK");
		}
		else
		{
			StringZ = GetFromDistributor(reqServer, i);

			if(StringZ[0] && !memcmp(message, StringZ, strlen(StringZ)))
			{
				messagesCount++;
			}
			WaitForDealerSignal(reqServer);
		}
	}

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, messagesCount);
	return messagesCount;
}

/**
 * CountSameMessageAgain
 * If in phase B the common message was received enough times then
 * 	in phase C redistribute it, else send empty string
*/
int CountSameMessageAgain(struct servers reqServer[], const char *message, int check)
{
	int tally = 1;
	char *StringZ;

	TraceInfo("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < numOfNodes; i++)
	{
		if (proc_id == i)
		{
			DistributorDistribute(reqServer, check ? message : "", i);
			TraceDebug("%s*Process:[%d] finished. Sending OK signal\n", __FUNCTION__, i);
			Distribute(reqServer, "OK");
		}
		else
		{
			StringZ = GetFromDistributor(reqServer, i);

			if(StringZ[0] && !memcmp(message, StringZ, strlen(StringZ)))
			{
				tally++;
			}
			WaitForDealerSignal(reqServer);
		}
	}

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, tally);
	return tally;
}

/**
 * Grade-Cast for Graded-VSS
*/
char *GradeCast(struct servers reqServer[], int distributor, const char *message, struct output array[])
{
	char *commonString = {0};
	char *result = (char*) malloc(StringSecreteSize);
	int tally = 0;
	int messagesCount = 0;

	memset(result, 0, StringSecreteSize);
	TraceInfo("%s*enter\n", __FUNCTION__);

	// Phase A
	commonString = GradeCastPhaseA(reqServer, distributor, message);

	// Phase B
	messagesCount = CountSameMessage(reqServer, commonString);

	Traitor(commonString);

	// Phase C
	if (messagesCount < (numOfNodes - badPlayers))
		tally = CountSameMessageAgain(reqServer, commonString, 0);
	else
		tally = CountSameMessageAgain(reqServer, commonString, 1);

	array[distributor] = ValidateTally(tally);
	TraceInfo("%s*exit*distributor[%d] output:code[%d] value:[%d]\n", __FUNCTION__, distributor, array[distributor].code, array[distributor].value);
	memcpy(result, commonString, strlen(commonString));
	return result;
}

