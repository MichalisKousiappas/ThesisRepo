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

static void *serverRoutine(void *context)
{
	const char *WhoAmI= "serverRoutine";

	printf("%s*enter\n", WhoAmI);

    reqServer[proc_id].value = zmq_socket(context, ZMQ_PUB);
	reqServer[proc_id].type = ZMQ_PUB;
    int rc = zmq_bind(reqServer[proc_id].value, serversIP[proc_id]);
	if (rc != 0)
	{
		printf("errno: [%d] [%s]\n", errno, strerror(errno));
	}
    assert(rc == 0);


    //  Socket to talk to main process so we know when to stop
    void *receiver = zmq_socket (context, ZMQ_PAIR);
	char receiverIP[25];
	sprintf(receiverIP, "inproc://server%d", proc_id);
    rc = zmq_connect(receiver, receiverIP);
	if (rc != 0)
	{
		printf("connect errno: [%d] [%s]\n", errno, strerror(errno));
	}
    assert(rc == 0);
	printf("%s*i got this far\n", WhoAmI);


	char sendBuffer[25];
	char areWeDone[6];
	int initialRandomNum = myRandomNum;
	memset(sendBuffer, 0, sizeof(sendBuffer));

	for(int i = 0; ; i++)
	{
		if (i == NUM_OF_NODES) i = 0;

		sprintf(sendBuffer, "%d %d", i+5, initialRandomNum);
		//printf("Sending data as client[%d] to [%d]: [%s]...\n", proc_id, i, sendBuffer);
		zmq_send(reqServer[proc_id].value, sendBuffer, 25, 0);
		memset(sendBuffer, 0, sizeof(sendBuffer));

//	 	zmq_recv(receiver, areWeDone, 5, ZMQ_DONTWAIT);
//		if (!memcmp(areWeDone, "done", 4))
//			break;
	}

	for(int i = 0; i < NUM_OF_NODES ; i++)
	{
		sprintf(sendBuffer, "%d %d", i+5, initialRandomNum);
		//printf("Sending data as client[%d] to [%d]: [%s]...\n", proc_id, i, sendBuffer);
		zmq_send(reqServer[proc_id].value, sendBuffer, 25, 0);
		memset(sendBuffer, 0, sizeof(sendBuffer));
	}

	// clean up your mess before exiting
	zmq_close(reqServer[proc_id].value);
	zmq_close(receiver);

	printf("%s*exit\n", WhoAmI);
	return NULL;
}

static void *workerRoutine(void *context)
{
	const char *WhoAmI= "workerRoutine";
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

		//printf("%s*iteration [%d]\n", WhoAmI, i);
	}


	char recvBuffer[26];
	int recvNumber;
	int dummy;
	memset(recvBuffer, 0, sizeof(recvBuffer));

	for(int i = 0; i < NUM_OF_NODES; i++)
	{
		if (i == proc_id) continue;

		printf("%s*awaiting to recieve something from [%s]\n", WhoAmI, serversIP[i]);
		zmq_recv(reqServer[i].value, recvBuffer, 25, 0);
		printf("Received data as server[%d]: [%s]...\n", proc_id, recvBuffer);
		sscanf(recvBuffer, "%d %d", &dummy, &recvNumber);
		myRandomNum += recvNumber;
		memset(recvBuffer, 0, sizeof(recvBuffer));
	}

	// clean up your mess before exiting
	for(int i = 0; i < NUM_OF_NODES; i++)
	{
		if ( i == proc_id) continue;
		zmq_close(reqServer[i].value);
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
		ip = strtok(hostsBuffer, " ");
		port = strtok(NULL, " ");
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

	// dirty solution to fix the server ip
	char *dummy = strtok(serversIP[proc_id], ":");
	dummy = strtok( NULL, ":");
	dummy = strtok( NULL, ":");

	//build the connection for the servert correctly
	sprintf(serversIP[proc_id], "tcp://*:%s", dummy);

	//  Socket to talk to server and inform it when workers are done
    void *updateServer = zmq_socket(context, ZMQ_PAIR);
	char updateServerIP[25];
	sprintf(updateServerIP, "inproc://server%d", proc_id);
    int rc = zmq_bind(updateServer, updateServerIP);
	assert(rc == 0);

    /* Initialize random number generator. use process number as seed
	 	add 3 because 0 and 1 produce the same number */
	srand(proc_id + 3);
	myRandomNum = rand() % 500;

	printf("process:[%d] random number:[%d] serverIP:[%s]\n", proc_id, myRandomNum, serversIP[proc_id]);

	pthread_t server, worker;
	pthread_create(&worker, NULL, workerRoutine, context);
	pthread_create(&server, NULL, serverRoutine, context);

	pthread_join(worker, NULL);
	zmq_send(updateServer, "done", 4, 0);
	printf("this is the end the final number is:[%d]\n", myRandomNum);
	pthread_join(server, NULL);

	fflush(stdout);
	zmq_close(updateServer);
	zmq_ctx_destroy(context);
	return 0;
}
