/*
 * Example with PUB/SUB
 * This example reads ports and hosts from the file hosts.txt with an attempt at threads
 * make sure the const NUM_OF_NODES is equal or less than the records of hosts.txt
 * to run all the processes at the same time replace NUM_OF_NODES and run this:
 * 	`for i in {0..NUM_OF_NODES - 1}; do ./multipub-sub $i > result$i.txt & done`
 */

#include <assert.h>
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

struct servers{
    int type;
    void *value;
};

#define NUM_OF_NODES 3
char serversIP[NUM_OF_NODES][256];
struct servers reqServer[NUM_OF_NODES];
int myRandomNum;
int proc_id;

static void *server_routine(void *context)
{
	const char *WhoAmI= "server_routine";

	printf("%s*enter\n", WhoAmI);

    reqServer[proc_id].value = zmq_socket(context, ZMQ_PUB);
	reqServer[proc_id].type = ZMQ_PUB;
    int rc = zmq_bind(reqServer[proc_id].value, serversIP[proc_id]);
	if (rc != 0)
	{
		printf("errno: [%d] [%s]\n", errno, strerror(errno));
	}
    assert(rc == 0);

	char sendBuffer[25];
	int initialRandomNum = myRandomNum;
	memset(sendBuffer, 0, sizeof(sendBuffer));

	for(int i = 0; ; i++)
	{
		if (i == NUM_OF_NODES) i = 0;

		sprintf(sendBuffer, "%d %d", i+5, initialRandomNum);
		//printf("Sending data as client[%d] to [%d]: [%s]...\n", proc_id, i, sendBuffer);
		zmq_send(reqServer[proc_id].value, sendBuffer, 25, 0);
		memset(sendBuffer, 0, sizeof(sendBuffer));
	}

	printf("%s*exit\n", WhoAmI);
	return NULL;
}

static void *worker_routine(void *context)
{
	const char *WhoAmI= "worker_routine";
	printf("%s*enter\n", WhoAmI);

	for(int i = 0; i < NUM_OF_NODES; i++)
	{
		if (i == proc_id) continue;

		reqServer[i].value = zmq_socket(context, ZMQ_SUB);
		reqServer[i].type = ZMQ_PULL;
		int rc = zmq_connect(reqServer[i].value, serversIP[i]);
		assert(rc == 0);

		char filter[25];
		sprintf(filter, "%d", i+5);
		printf("%s*proc:[%d] filter:[%s]\n", WhoAmI, i, filter);
		zmq_setsockopt(reqServer[i].value, ZMQ_SUBSCRIBE, filter, strlen(filter));

		printf("%s*iteration [%d]\n", WhoAmI, i);
	}


	char recvBuffer[26];
	memset(recvBuffer, 0, sizeof(recvBuffer));

	for(int i = 0; i < NUM_OF_NODES; i++)
	{
		if (i == proc_id) continue;

		printf("%s*yes?[%s]\n", WhoAmI, serversIP[i]);
		zmq_recv(reqServer[i].value, recvBuffer, 25, 0);
		printf("Received data as server[%d]: [%s]...\n", proc_id, recvBuffer);
		myRandomNum += atoi(recvBuffer);
		memset(recvBuffer, 0, sizeof(recvBuffer));
	}

	printf("%s*exit\n", WhoAmI);
	return NULL;
}

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
		//printf("[%s]\n", hostsBuffer);
		ip = strtok(hostsBuffer, " ");
		//printf("[%s]\n", ip);
		port = strtok(NULL, " ");
		//printf("[%s]\n", port);
		sprintf(serversIP[i], "tcp://%s:%s", ip, port);
	}
}

int main (int argc,char *argv[])
{
	const char *WhoAmI= "main";

	if (argc != 2)
	{
		printf("not enough arguments\n");
		return -1;
	}

	printf("%s*enter\n", WhoAmI);

	proc_id = atoi(argv[1]);
	void *context = zmq_ctx_new ();

	init();

	// dirty solution to fix the server
	char *dummy = strtok(serversIP[proc_id], ":");
	dummy = strtok( NULL, ":");
	dummy = strtok( NULL, ":");

	//build the connection for the servert correctly
	sprintf(serversIP[proc_id], "tcp://*:%s", dummy);

    // Initialize random number generator. use process number as seed
	srand(proc_id + 3);
	myRandomNum = rand() % 500;

	printf("process:[%d] random number:[%d] serverIP:[%s]\n", proc_id, myRandomNum, serversIP[proc_id]);

	pthread_t server;
	pthread_t worker;
	pthread_create(&worker, NULL, worker_routine, context);
	pthread_create(&server, NULL, server_routine, context);

	pthread_join(worker, NULL);
	pthread_join(server, NULL);
	printf("this is the end\n");
	for(int i = 0; i < NUM_OF_NODES; i++) zmq_close(reqServer[i].value);
	fflush(stdout);
	zmq_ctx_destroy(context);
	return 0;
}
