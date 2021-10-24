/*
 * Example with PUSH/PULL
 * This example reads ports and hosts from the file hosts.txt
 * make sure the const NUM_OF_NODES is equal or less than the records of hosts.txt
 * to run all the processes at the same time replace NUM_OF_NODES and run this:
 * 	`for i in {0..NUM_OF_NODES - 1}; do ./multiProcesses $i NUM_OF_NODES > result$i.txt & done`
 */

#include <assert.h>
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

//use this bad boy so printf are printed on demand and not always
#ifdef DEBUG
#define debug(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...) ((void)0)
#endif

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

/**
  Send the same message to all other nodes
 */
void Distribute(struct servers reqServer[], const char *secret)
{
	char sendBuffer [15];
	char recvBuffer [15];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(recvBuffer, 0, sizeof(recvBuffer));

	if (proc_id%3 == 0)
		sprintf(sendBuffer, "%s", "0011011");
	else
		sprintf(sendBuffer, "%s", secret);

	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		printf("Sending data as client[%d] to [%d]: [%s]...\n", proc_id, i, sendBuffer);
		zmq_send(reqServer[i].value, sendBuffer, 10, 0);

		zmq_recv(reqServer[proc_id].value, recvBuffer, 10, 0);
		printf("Received data as server[%d]: [%s]...\n", proc_id, recvBuffer);

		// Count the messages that match yours
		if(sendBuffer[0] && !memcmp(sendBuffer, recvBuffer, strlen(sendBuffer)))
		{
			tally++;
		}
		memset(recvBuffer, 0, sizeof(recvBuffer));
	}
}

/*
   Graded-Cast tally validation
 */
void ValidateTally()
{
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
	debug("preparing connections\n");
	for(int i = 0; i <= numOfNodes; i++)
	{
		if (i == proc_id) continue;

		reqServer[i].value = zmq_socket(context, ZMQ_PUSH);
		reqServer[i].type = ZMQ_PUSH;
		zmq_connect(reqServer[i].value, serversIP[i]);
	}
	debug("done\n");
}

char *GetFromDealer(struct servers reqServer[])
{
	debug("GetFromDealer enter\n");
	char *result = (char*) malloc(15);
	char sendBuffer [15];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(result, 0, sizeof(15));

	sprintf(sendBuffer, "%d", proc_id);

	printf("Sending data as client[%d] to dealer: [%s]...\n", proc_id, sendBuffer);
	zmq_send(reqServer[dealer].value, sendBuffer, 10, 0);

	zmq_recv(reqServer[dealer].value, result, 10, 0);
	printf("Received data from dealer: [%s]...\n", result);

	return result;
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
		debug("[%s]\n", hostsBuffer);
		ip = strtok(hostsBuffer, " ");
		debug("[%s]\n", ip);
		port = strtok(NULL, " ");
		debug("[%s]\n", port);
		sprintf(serversIP[i], "tcp://%s:%s", ip, port);
	}
}

int main (int argc,char *argv[])
{
	if (argc != 3)
	{
		printf("not enough arguments\n");
		return -1;
	}

	proc_id = atoi(argv[1]);
	numOfNodes= atoi(argv[2]);
	dealer = numOfNodes;
	badPlayers = numOfNodes/3;
	char serversIP[numOfNodes][256];
	struct servers reqServer[numOfNodes];
	char *secret;

	debug("proc_id:[%d] numOfNodes:[%d] dealer:[%d] badPlayers:[%d]\n", proc_id, numOfNodes, dealer, badPlayers);
	void *context = zmq_ctx_new();

	// Initialize
	init(serversIP);

	// dirty solution to fix the server
	char *dummy = strtok(serversIP[proc_id], ":");
	dummy = strtok( NULL, ":");
	dummy = strtok( NULL, ":");

	// build the connection for the servert correctly
	sprintf(serversIP[proc_id], "tcp://*:%s", dummy);

	printf("%d: %s\n", proc_id, serversIP[proc_id]);
	reqServer[proc_id].value = zmq_socket(context, ZMQ_PULL);
	reqServer[proc_id].type = ZMQ_PULL;
	int rc = zmq_bind(reqServer[proc_id].value, serversIP[proc_id]);
	assert(rc == 0);

	//connect to all other nodes
	PrepareConnections(context, reqServer, serversIP);

	secret = GetFromDealer(reqServer);
	Distribute(reqServer, secret);
	ValidateTally();

	printf("process[%d] output:code[%d] value:[%d]\n", proc_id, out.code, out.value);

	for(int i = 0; i <= numOfNodes; i++) zmq_close(reqServer[i].value);
	fflush(stdout);
	zmq_ctx_destroy(context);
	return 0;
}
