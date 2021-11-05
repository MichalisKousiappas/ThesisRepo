/*
 * Example with PUSH/PULL
 * This example reads ports and hosts from the file hosts.txt
 * make sure the const NUM_OF_NODES is equal or less than the records of hosts.txt
 * to run all the processes at the same time replace NUM_OF_NODES and run this:
 * 	`for i in {0..NUM_OF_NODES - 1}; do ./multiProcesses $i NUM_OF_NODES dealer> result$i.dmp & done`
 */
#include <assert.h>
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

//use this bad boy so printf are printed on demand and not always. fflush is to force the output in case we write to file through bash
#ifdef DEBUG
#define TraceDebug(fmt, ...) fprintf(stdout,"DEBUG " "%s %d " fmt, GetTime(), getpid(), ##__VA_ARGS__); fflush(stdout)
#else
#define TraceDebug(fmt, ...) ((void)0)
#endif
//use this bad boy instaed of printf for better formatting. fflush is to force the output in case we write to file through bash
#define TraceInfo(fmt, ...)	fprintf(stdout,"INFO  " "%s %d " fmt, GetTime(), getpid(), ##__VA_ARGS__); fflush(stdout)

struct output
{
	int code;
	int value;
};

struct servers{
	int type;
	void *value;
};

//Global Variables. This are global since they are for the whole process
int numOfNodes;
int dealer;
int tally = 1; //count yourself
int proc_id;
int badPlayers;
struct output out = {0, 0};

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

	TraceDebug("%s*enter\n", __FUNCTION__);

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
	TraceDebug("%s*exit\n", __FUNCTION__);
}

void GetMessages(struct servers reqServer[], const char *secret)
{
	char recvBuffer [15];

	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceDebug("%s*enter\n", __FUNCTION__);

	//Distribute your message to all other nodes
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
	TraceDebug("%s*exit\n", __FUNCTION__);
}

/*
   Graded-Cast tally validation
 */
void ValidateTally()
{
	TraceDebug("%s*tally:[%d]\n", __FUNCTION__, tally);
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
	TraceDebug("%s*enter\n", __FUNCTION__);
	for(int i = 0; i <= numOfNodes; i++)
	{
		if (i == proc_id || (proc_id == dealer && i == numOfNodes)) continue;

		reqServer[i].value = zmq_socket(context, ZMQ_PUSH);
		reqServer[i].type = ZMQ_PUSH;
		zmq_connect(reqServer[i].value, serversIP[i]);
	}
	TraceDebug("%s*exit\n", __FUNCTION__);
}

char *GetFromDealer(struct servers reqServer[])
{
	TraceDebug("%s*enter\n", __FUNCTION__);
	char *result = (char*) malloc(15);
	char sendBuffer [15];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(result, 0, sizeof(15));

	sprintf(sendBuffer, "%d", proc_id);

	TraceInfo("Sending data as client[%d] to dealer: [%s]\n", proc_id, sendBuffer);
	zmq_send(reqServer[numOfNodes].value, sendBuffer, 10, 0);

	zmq_recv(reqServer[proc_id].value, result, 10, 0);
	TraceInfo("Received data from dealer: [%s]\n", result);

	TraceDebug("%s*exit\n", __FUNCTION__);
	return result;
}

/**
  Send the same message to all other nodes
 */
void DealerDistribute(struct servers reqServer[])
{
	char sendBuffer [15];
	char recvBuffer [15];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceDebug("%s*enter\n", __FUNCTION__);
	// "Secret" binary string
	sprintf(sendBuffer, "%s", "110011011");

	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		zmq_recv(reqServer[numOfNodes].value, recvBuffer, 10, 0);
		TraceInfo("Received data as dealer: [%s]\n", recvBuffer);
		memset(recvBuffer, 0, sizeof(recvBuffer));

		zmq_send(reqServer[i].value, sendBuffer, 10, 0);
		TraceInfo("Send data as dealer to [%d]: [%s]\n", i, sendBuffer);
	}
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

void ValidateInput()
{
	if (dealer > numOfNodes -1 || dealer < 0)
	{
		printf("dealer process id not valid\n");
		exit(-2);
	}

	if (proc_id > numOfNodes -1 || proc_id < 0)
	{
		printf("process id not valid\n");
		exit(-3);
	}
	
	if (numOfNodes < 2)
	{
		printf("number of nodes must be greater than 1\n");
		exit(-4);
	}
}

int main (int argc,char *argv[])
{
	if (argc != 4)
	{
		printf("not enough arguments\n");
		printf("Usage: Graded-Cast.o <proc id> <number of nodes> <dealer\n");
		return -1;
	}

	proc_id = atoi(argv[1]);
	numOfNodes= atoi(argv[2]);
	dealer = atoi(argv[3]);
	badPlayers = numOfNodes/3;
	char serversIP[numOfNodes+1][256];
	struct servers reqServer[numOfNodes+1];
	char *secret = {0};

	TraceDebug("proc_id:[%d] numOfNodes:[%d] dealer:[%d] badPlayers:[%d]\n", proc_id, numOfNodes, dealer, badPlayers);
	void *context = zmq_ctx_new();

	// Initialize
	init(serversIP);

	// dirty solution to fix the server
	char *dummy = strtok(serversIP[proc_id], ":");
	dummy = strtok( NULL, ":");
	dummy = strtok( NULL, ":");

	// build the connection for the servert correctly
	sprintf(serversIP[proc_id], "tcp://*:%s", dummy);

	TraceInfo("%d: %s\n", proc_id, serversIP[proc_id]);
	reqServer[proc_id].value = zmq_socket(context, ZMQ_PULL);
	reqServer[proc_id].type = ZMQ_PULL;
	int rc = zmq_bind(reqServer[proc_id].value, serversIP[proc_id]);
	assert(rc == 0);

	if (proc_id == dealer)
	{
		// dirty solution to fix the server
		char *dummy = strtok(serversIP[numOfNodes], ":");
		dummy = strtok( NULL, ":");
		dummy = strtok( NULL, ":");

		// build the connection for the servert correctly
		sprintf(serversIP[numOfNodes], "tcp://*:%s", dummy);

		TraceInfo("%d is the dealer and listens on: %s\n", proc_id, serversIP[numOfNodes]);
		reqServer[numOfNodes].value = zmq_socket(context, ZMQ_PULL);
		reqServer[numOfNodes].type = ZMQ_PULL;
		int rc = zmq_bind(reqServer[numOfNodes].value, serversIP[numOfNodes]);
		assert(rc == 0);
	}

	//connect to all other nodes
	PrepareConnections(context, reqServer, serversIP);

	if (proc_id == dealer)
	{
		char dummy[] = "110011011";
		secret = dummy;
		DealerDistribute(reqServer);
	}
	else
		secret = GetFromDealer(reqServer);

	sleep(1); //artificial delay so the dealer can distribute the secret to everyone

	Distribute(reqServer, secret);
	GetMessages(reqServer, secret);
	ValidateTally();

	TraceInfo("process[%d] output:code[%d] value:[%d]\n", proc_id, out.code, out.value);

	for(int i = 0; i <= numOfNodes; i++)
	{
		TraceDebug("Closed connection[%d]\n", i);
		zmq_close(reqServer[i].value);
	}
	zmq_ctx_destroy(context);
	TraceInfo("finished\n");
	return 0;
}