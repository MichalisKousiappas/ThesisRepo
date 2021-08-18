/*
 * Example with PUSH/PULL
 * This example reads ports and hosts from the file hosts.txt
 * make sure the const NUM_OF_NODES is equal or less than the records of hosts.txt
 * to run all the processes at the same time replace NUM_OF_NODES and run this:
 * 	`for i in {0..NUM_OF_NODES - 1}; do ./multiProcesses $i > result$i.txt & done`
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
#define debug(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...) ((void)0)
#endif

struct servers{
    int type;
    void *value;
};

#define NUM_OF_NODES 18
char serversIP[NUM_OF_NODES][256];

void init()
{
	FILE *file;
	char hostsBuffer[150];
	char *port;
	char *ip;

	if (!(file = fopen("hosts.txt","r")))
	{
		perror("Could not open file");exit(-1);
	}

	for(int i = 0; i < NUM_OF_NODES; i++)
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

	int proc_id = atoi(argv[1]);
	void *context = zmq_ctx_new ();
	struct servers reqServer[NUM_OF_NODES];

	init();

	// dirty solution to fix the server
	char *dummy = strtok(serversIP[proc_id], ":");
	dummy = strtok( NULL, ":");
	dummy = strtok( NULL, ":");

	//build the connection for the servert correctly
	sprintf(serversIP[proc_id], "tcp://*:%s", dummy);

	printf("%d: %s\n", proc_id, serversIP[proc_id]);
    reqServer[proc_id].value = zmq_socket(context, ZMQ_PULL);
	reqServer[proc_id].type = ZMQ_PULL;
    int rc = zmq_bind(reqServer[proc_id].value, serversIP[proc_id]);
    assert(rc == 0);

	for(int i = 0; i < NUM_OF_NODES; i++)
	{
		if (i == proc_id) continue;

		reqServer[i].value = zmq_socket(context, ZMQ_PUSH);
		reqServer[proc_id].type = ZMQ_PUSH;
		zmq_connect(reqServer[i].value, serversIP[i]);
	}

    // Initialize random number generator. use process number as seed
	srand(proc_id);
	int myRandomNum = rand() % 500;

	char sendBuffer [10] = {0};
	char recvBuffer [10] = {0};
	sprintf(sendBuffer, "%d", myRandomNum);
	printf("process[%d] random number:[%d]\n", proc_id, myRandomNum);

	for (int i = 0; i < NUM_OF_NODES; i++)
	{
		if (i == proc_id) continue;
		printf("Sending data as client[%d] to [%d]: [%d]...\n", proc_id, i, myRandomNum);
		zmq_send(reqServer[i].value, sendBuffer, 5, 0);

		zmq_recv(reqServer[proc_id].value, recvBuffer, 5, 0);
		printf("Received data as server[%d]: [%s]...\n", proc_id, recvBuffer);
		myRandomNum += atoi(recvBuffer);
		memset(recvBuffer, 0, sizeof(recvBuffer));
	}

	printf("The final number for process[%d] is:[%d]\n", proc_id, myRandomNum);

	for(int i = 0; i < NUM_OF_NODES; i++) zmq_close(reqServer[i].value);
	fflush(stdout);
	zmq_ctx_destroy(context);
	return 0;
}
