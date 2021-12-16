#include "functions.h"

/**
  Send the same message to all other nodes
 */
void Distribute(struct servers reqServer[], const char *secret)
{
	char sendBuffer[SECRETE_SIZE];

	memset(sendBuffer, 0, sizeof(sendBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);

	if ((proc_id%3 == 0) && (proc_id != 0) && (proc_id != dealer))
	{
		sprintf(sendBuffer, "%s", "0011011");
		TraceDebug("%s*I am a traitor hahaha[%d]\n", __FUNCTION__, proc_id);
	}
	else
		sprintf(sendBuffer, "%s", secret);

	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		TraceInfo("Sending data as client[%d] to [%d]: [%s]\n", proc_id, i, sendBuffer);
		zmq_send(reqServer[i].value, sendBuffer, SECRETE_SIZE, 0);
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}

void GetMessages(struct servers reqServer[], const char *secret)
{
	char recvBuffer[SECRETE_SIZE];

	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		zmq_recv(reqServer[proc_id].value, recvBuffer, SECRETE_SIZE, 0);
		TraceInfo("Received data as server[%d]: [%s]\n", proc_id, recvBuffer);

		// Count the messages that match yours
		if(recvBuffer[0] && !memcmp(secret, recvBuffer, strlen(recvBuffer)))
		{
			tally++;
		}
		memset(recvBuffer, 0, sizeof(recvBuffer));
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}

/*
   Graded-Cast tally validation
 */
void ValidateTally()
{
	TraceInfo("%s*tally:[%d]\n", __FUNCTION__, tally);
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

	//reset tally
	tally = 1;
}

char *GetFromDistributor(struct servers reqServer[], int distributor)
{
	TraceInfo("%s*enter\n", __FUNCTION__);
	char *result = (char*) malloc(SECRETE_SIZE);
	char sendBuffer [SECRETE_SIZE];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(result, 0, sizeof(SECRETE_SIZE));

	sprintf(sendBuffer, "%d", proc_id);

	TraceInfo("Sending data as client[%d] to dealer: [%s]\n", proc_id, sendBuffer);
	zmq_send(reqServer[distributor].value, sendBuffer, SECRETE_SIZE, 0);

	zmq_recv(reqServer[proc_id].value, result, SECRETE_SIZE, 0);
	TraceInfo("Received secret from dealer: [%s]\n", result);

	TraceInfo("%s*exit\n", __FUNCTION__);
	return result;
}

/**
  Send the same message to all other nodes
 */
void DistributorDistribute(struct servers reqServer[], const char *secret, int distributor)
{
	char sendBuffer [SECRETE_SIZE];
	char recvBuffer [SECRETE_SIZE];
	int requestor;

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);
	// "Secret" binary string
	sprintf(sendBuffer, "%s", secret);

	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		zmq_recv(reqServer[distributor].value, recvBuffer, SECRETE_SIZE, 0);
		TraceInfo("Received data as dealer: [%s]\n", recvBuffer);

		requestor = atoi(recvBuffer);
		zmq_send(reqServer[requestor].value, sendBuffer, SECRETE_SIZE, 0);
		TraceInfo("Send data as dealer to [%d]: [%s]\n", requestor, sendBuffer);

		memset(recvBuffer, 0, sizeof(recvBuffer));
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}

void GradeCast(struct servers reqServer[], int distributor)
{
	char temp[SECRETE_SIZE] = {0};
	char *secret = {0};

	TraceInfo("%s*enter\n", __FUNCTION__);

	if (proc_id == distributor)
	{
		sprintf(temp, "%d%s", distributor, "110011011");
		TraceDebug("%s*secret is:[%s]\n", __FUNCTION__, temp);
		secret = temp;
		DistributorDistribute(reqServer, secret, distributor);
	}
	else
	{
		secret = GetFromDistributor(reqServer, distributor);
	}

	sleep(1); //artificial delay so the dealer can distribute the secret to everyone

	Distribute(reqServer, secret);
	GetMessages(reqServer, secret);
	ValidateTally();

	TraceInfo("process[%d] output:code[%d] value:[%d]\n", proc_id, out.code, out.value);
	TraceInfo("%s*exit\n", __FUNCTION__);
}