#include "functions.h"

char *GetTime()
{
	time_t t;
	struct timeval timeVar;
	char *buf = (char *) malloc(25);
	memset(buf, 0, 40);

	gettimeofday(&timeVar, NULL);
	time(&t);
    strftime(buf, 21, "%d/%m/%Y %T", localtime(&t));
	sprintf(buf+19, ".%ld", timeVar.tv_usec);
	buf[23] = '\0'; //force null otherwise it will print more than 3 digits
	return buf;
}

/**
  Send the same message to all other nodes
 */
void Distribute(struct servers reqServer[], const char *secret)
{
	char sendBuffer [15];

	memset(sendBuffer, 0, sizeof(sendBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);

	if ((proc_id%3 == 0) && (proc_id != 0) && (proc_id != dealer))
		sprintf(sendBuffer, "%s", "0011011");
	else
		sprintf(sendBuffer, "%s", secret);

	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		TraceInfo("Sending data as client[%d] to [%d]: [%s]\n", proc_id, i, sendBuffer);
		zmq_send(reqServer[i].value, sendBuffer, 10, 0);
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}

void GetMessages(struct servers reqServer[], const char *secret)
{
	char recvBuffer [15];

	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		zmq_recv(reqServer[proc_id].value, recvBuffer, 10, 0);
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
}

/*
   Prepare the connections to other nodes
 */
void PrepareConnections(void *context, struct servers reqServer[], char serversIP[][256])
{
	TraceInfo("%s*enter\n", __FUNCTION__);
	for(int i = 0; i <= numOfNodes; i++)
	{
		if (i == proc_id || (proc_id == dealer && i == numOfNodes)) continue;

		reqServer[i].value = zmq_socket(context, ZMQ_PUSH);
		reqServer[i].type = ZMQ_PUSH;
		zmq_connect(reqServer[i].value, serversIP[i]);
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}

char *GetFromDealer(struct servers reqServer[])
{
	TraceInfo("%s*enter\n", __FUNCTION__);
	char *result = (char*) malloc(15);
	char sendBuffer [15];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(result, 0, sizeof(15));

	sprintf(sendBuffer, "%d", proc_id);

	TraceInfo("Sending data as client[%d] to dealer: [%s]\n", proc_id, sendBuffer);
	zmq_send(reqServer[numOfNodes].value, sendBuffer, 10, 0);

	zmq_recv(reqServer[proc_id].value, result, 10, 0);
	TraceInfo("Received secret from dealer: [%s]\n", result);

	TraceInfo("%s*exit\n", __FUNCTION__);
	return result;
}

/**
  Send the same message to all other nodes
 */
void DealerDistribute(struct servers reqServer[], const char *secret)
{
	char sendBuffer [15];
	char recvBuffer [15];
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
		zmq_recv(reqServer[numOfNodes].value, recvBuffer, 10, 0);
		TraceInfo("Received data as dealer: [%s]\n", recvBuffer);

		requestor = atoi(recvBuffer);
		zmq_send(reqServer[requestor].value, sendBuffer, 10, 0);
		TraceInfo("Send data as dealer to [%d]: [%s]\n", requestor, sendBuffer);

		memset(recvBuffer, 0, sizeof(recvBuffer));
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}

/*
	Read IP and port from hosts file and fill the
	the serversIP with the correct values
 */
void init(char serversIP[][256])
{
	FILE *file;
	char hostsBuffer[150];
	char *port;
	char *ip;

	if (!(file = fopen("hosts.txt","r")))
	{
		perror("Could not open file");exit(-1);
	}

	for(int i = 0; i <= numOfNodes; i++)
	{
		if (!(fgets(hostsBuffer, 150, file)))
		{
			perror("Could not read from file");exit(-2);
		}

		hostsBuffer[strcspn(hostsBuffer, "\n")] = 0;
		TraceDebug("[%d] [%s]\n", i, hostsBuffer);
		ip = strtok(hostsBuffer, " ");
		TraceDebug("\t[%s]\n", ip);
		port = strtok(NULL, " ");
		TraceDebug("\t[%s]\n", port);
		sprintf(serversIP[i], "tcp://%s:%s", ip, port);
	}
}

void ValidateInput(int argc)
{
	if (argc != 4)
	{
		printf("not enough arguments\n");
		printf("Usage: Graded-Cast.o <proc id> <number of nodes> <dealer>\n");
		exit(-1);
	}

	if ((dealer > numOfNodes -1) || (dealer < 0))
	{
		printf("dealer process id not valid\n");
		exit(-3);
	}

	if ((proc_id > numOfNodes -1) || (proc_id < 0))
	{
		printf("process id not valid\n");
		exit(-4);
	}
	
	if (numOfNodes < 2)
	{
		printf("number of nodes must be greater than 1\n");
		exit(-5);
	}
}