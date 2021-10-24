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
void DealerDistribute(struct servers reqServer[])
{
	char sendBuffer [15];
	char recvBuffer [15];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(recvBuffer, 0, sizeof(recvBuffer));

	// "Secret" binary string
	sprintf(sendBuffer, "%s", "110011011");

	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		zmq_recv(reqServer[i].value, recvBuffer, 10, 0);
		printf("Received data as dealer: [%s]...\n", recvBuffer);

		printf("Sending data as dealer to [%d]: [%s]...\n", i, sendBuffer);
		zmq_send(reqServer[i].value, sendBuffer, 10, 0);

		memset(recvBuffer, 0, sizeof(recvBuffer));
	}
}

/*
   Prepare the connections to other nodes
 */
void PrepareConnections(void *context, struct servers reqServer[], char serversIP[][256])
{
	for(int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		reqServer[i].value = zmq_socket(context, ZMQ_PUSH);
		reqServer[i].type = ZMQ_PUSH;
		zmq_connect(reqServer[i].value, serversIP[i]);
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
	if (argc != 2)
	{
		printf("not enough arguments\n");
		return -1;
	}

	numOfNodes= atoi(argv[1]);
	proc_id = numOfNodes; //dealers id is always the last one for consistency
	dealer = numOfNodes;
	badPlayers = numOfNodes/3;
	char serversIP[numOfNodes][256];
	struct servers reqServer[numOfNodes];

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

	//force output
	fflush(stdout);

	DealerDistribute(reqServer);

	for(int i = 0; i < numOfNodes; i++) zmq_close(reqServer[i].value);
	fflush(stdout);
	zmq_ctx_destroy(context);
	return 0;
}
