/*
 * Dealer process for Grade-Cast
 * Can be run either before or after Grade-Cast for loop
 */

#include <assert.h>
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

//use this bad boy so printf are printed on demand and not always
#ifdef DEBUG
#define TraceDebug(fmt, ...) fprintf(stdout,"DEBUG " "%s %d " fmt, GetTime(), getpid(), ##__VA_ARGS__)
#else
#define TraceDebug(fmt, ...) ((void)0)
#endif
//use this bad boy instaed of printf for better formatting
#define TraceInfo(fmt, ...) fprintf(stdout,"INFO  " "%s %d " fmt, GetTime(), getpid(), ##__VA_ARGS__)

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
		//if (i == proc_id) continue;
		zmq_recv(reqServer[proc_id].value, recvBuffer, 10, 0);
		TraceInfo("Received data as dealer: [%s]...\n", recvBuffer);
		memset(recvBuffer, 0, sizeof(recvBuffer));

		zmq_send(reqServer[i].value, sendBuffer, 10, 0);
		TraceInfo("Send data as dealer to [%d]: [%s]...\n", i, sendBuffer);
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
		TraceDebug("[%d] [%s]\n", i, hostsBuffer);
		ip = strtok(hostsBuffer, " ");
		TraceDebug("\t[%s]\n", ip);
		port = strtok(NULL, " ");
		TraceDebug("\t[%s]\n", port);
		sprintf(serversIP[i], "tcp://%s:%s", ip, port);
	}
}

int main (int argc,char *argv[])
{
	if (argc != 2)
	{
		printf("not enough arguments\n");
		printf("Usage: dealerProc.o <number of nodes>\n");
		return -1;
	}

	numOfNodes= atoi(argv[1]);
	proc_id = numOfNodes; //dealers id is always the last one for consistency
	dealer = numOfNodes;
	badPlayers = numOfNodes/3;
	char serversIP[numOfNodes+1][256];
	struct servers reqServer[numOfNodes+1];

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

	//connect to all other nodes
	PrepareConnections(context, reqServer, serversIP);

	//force output
	fflush(stdout);

	DealerDistribute(reqServer);
	TraceInfo("dealer process finsihed successfully\n");

	for(int i = 0; i <= numOfNodes; i++) zmq_close(reqServer[i].value);
	fflush(stdout);
	zmq_ctx_destroy(context);
	return 0;
}
