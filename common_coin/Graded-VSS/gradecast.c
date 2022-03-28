#include "gradecast.h"

// Local Function Declaration
void DistributorDistribute(struct servers reqServer[], const char *secret, int distributor);
void GetFromDistributor(struct servers reqServer[], int distributor, char result[]);

/*
   Graded-Cast tally validation
 */
struct output ValidateTally(int tally)
{
	TraceDebug("%s*tally:[%d]\n", __FUNCTION__, tally);
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
void GetFromDistributor(struct servers reqServer[], int distributor, char result[])
{
	TraceDebug("%s*enter\n", __FUNCTION__);
	char sendBuffer[8];

	memset(sendBuffer, 0, sizeof(sendBuffer));

	sprintf(sendBuffer, "%d", proc_id);

	TraceDebug("Sending data as client[%d] to distributor[%d]: [%s]\n", proc_id, distributor, sendBuffer);
	zmq_send(reqServer[distributor].value, sendBuffer, strlen(sendBuffer), 0);

	zmq_recv(reqServer[distributor].value, result, StringSecreteSize, 0);
	TraceDebug("Received secret from distributor: [%s]\n", result);

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
  Send the same message to all other nodes
 */
void DistributorDistribute(struct servers reqServer[], const char *secret, int distributor)
{
	char sendBuffer[StringSecreteSize + 1];
	char recvBuffer[8];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceDebug("%s*enter\n", __FUNCTION__);
	// "Secret" binary string
	sprintf(sendBuffer, "%s", secret);

	messages++;
	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;

		zmq_recv(reqServer[proc_id].value, recvBuffer, sizeof(recvBuffer) - 1, 0);
		TraceDebug("Received confirmation as distributor[%d]: [%s]\n", distributor, recvBuffer);

		TraceDebug("Sending data as distributor to [%d]: [%s]\n", atoi(recvBuffer), sendBuffer);
		zmq_send(reqServer[proc_id].value, sendBuffer, StringSecreteSize, 0);

		memset(recvBuffer, 0, sizeof(recvBuffer));
		messages++;
	}
	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * Grade-Cast Phase A
 * In phase A, the distributor will send his message to all other processes.
*/
void GradeCastPhaseA(struct servers reqServer[], int distributor, const char *message, char result[])
{
	TraceDebug("%s*enter\n", __FUNCTION__);

	if (proc_id == distributor)
	{
		DistributorDistribute(reqServer, message, distributor);
		TraceDebug("%s*distirbutor:[%d] finished. Sending OK signal\n", __FUNCTION__, distributor);
		memcpy(result, message, strlen(message));
		Distribute(reqServer, "OK");
	}
	else
	{
		GetFromDistributor(reqServer, distributor, result);
		WaitForDealerSignal(reqServer);
	}

	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 * CountSameMessage
 * All processes take turn and send their message to all other nodes.
 * if its not their turn to send they wait for the distributor processes to send.
*/
int CountSameMessage(struct servers reqServer[], const char *message)
{
	int messagesCount = 1;
	char StringZ[StringSecreteSize + 1];

	TraceDebug("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < numOfNodes; i++)
	{
		memset(StringZ, 0, sizeof(StringZ));
		if (proc_id == i)
		{
			DistributorDistribute(reqServer, message, i);
			TraceDebug("%s*Process:[%d] finished. Sending OK signal\n", __FUNCTION__, i);
			Distribute(reqServer, "OK");
		}
		else
		{
			GetFromDistributor(reqServer, i, StringZ);

			if(StringZ[0] && !memcmp(message, StringZ, strlen(StringZ)))
			{
				messagesCount++;
			}
			WaitForDealerSignal(reqServer);
		}
	}

	TraceDebug("%s*exit[%d]\n", __FUNCTION__, messagesCount);
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
	char StringZ[StringSecreteSize + 1];

	TraceDebug("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < numOfNodes; i++)
	{
		memset(StringZ, 0, sizeof(StringZ));
		if (proc_id == i)
		{
			DistributorDistribute(reqServer, check ? message : "", i);
			TraceDebug("%s*Process:[%d] finished. Sending OK signal\n", __FUNCTION__, i);
			Distribute(reqServer, "OK");
		}
		else
		{
			GetFromDistributor(reqServer, i, StringZ);

			if(StringZ[0] && !memcmp(message, StringZ, strlen(StringZ)))
			{
				tally++;
			}
			WaitForDealerSignal(reqServer);
		}
	}

	TraceDebug("%s*exit[%d]\n", __FUNCTION__, tally);
	return tally;
}

/**
 * Grade-Cast for Graded-VSS
*/
void GradeCast(struct servers reqServer[], int distributor, const char *message, struct output array[], char result[])
{
	int tally = 0;
	int messagesCount = 0;

	TraceDebug("%s*enter\n", __FUNCTION__);

	// Phase A
	GradeCastPhaseA(reqServer, distributor, message, result);

	// Phase B
	messagesCount = CountSameMessage(reqServer, result);

	Traitor(result);

	// Phase C
	if (messagesCount < (numOfNodes - badPlayers))
		tally = CountSameMessageAgain(reqServer, result, 0);
	else
		tally = CountSameMessageAgain(reqServer, result, 1);

	array[distributor] = ValidateTally(tally);
	TraceDebug("%s*exit*distributor[%d] output:code[%d] value:[%d]\n", __FUNCTION__, distributor, array[distributor].code, array[distributor].value);
}

