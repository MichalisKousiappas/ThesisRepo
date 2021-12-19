#include "init.h"

/*
	Read IP and port from hosts file and fill the
	the serversIP with the correct values
 */
void ReadIPFromFile(char serversIP[][256], char *filename)
{
	FILE *file;
	char hostsBuffer[150];
	char *port;
	char *ip;

	if (!(file = fopen(filename,"r")))
	{
		perror("Could not open file");exit(-1);
	}

	for(int i = 0; i < numOfNodes; i++)
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

	// dirty solution to fix the server
	char *dummy = strtok(serversIP[proc_id], ":");
	dummy = strtok( NULL, ":");
	dummy = strtok( NULL, ":");

	// build the connection for the server correctly
	sprintf(serversIP[proc_id], "tcp://*:%s", dummy);
}

/*
   Prepare the connections to other nodes
 */
void PrepareConnections(void *context, struct servers reqServer[], char serversIP[][256])
{
	TraceInfo("%s*enter\n", __FUNCTION__);

	//Create connection to communicate with other processes
	for(int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id || (proc_id == dealer && i == numOfNodes)) continue;

		reqServer[i].value = zmq_socket(context, ZMQ_PUSH);
		reqServer[i].type = ZMQ_PUSH;
		zmq_connect(reqServer[i].value, serversIP[i]);
	}

	//Create connection for others to communicate with you
	TraceDebug("%s*%d: %s\n", __FUNCTION__, proc_id, serversIP[proc_id]);
	reqServer[proc_id].value = zmq_socket(context, ZMQ_PULL);
	reqServer[proc_id].type = ZMQ_PULL;
	int rc = zmq_bind(reqServer[proc_id].value, serversIP[proc_id]);
	assert(rc == 0);

	TraceInfo("%s*exit\n", __FUNCTION__);
}

/**
 * Check validity of input
*/
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

/**
 * Initialize variables with the correct values
*/
void init(void *context, 
		struct servers reqServer[],
		struct servers syncServer[],
		char serversIP[][256],
		char syncIP[][256],
		double polynomials[CONFIDENCE_PARAM][badPlayers],
		double polyEvals[numOfNodes][CONFIDENCE_PARAM])
{
	char filename[35] = "hosts.txt";
	// Fill serversIP
	ReadIPFromFile(serversIP, filename);

	memcpy(filename, "private.txt", sizeof(filename));
	// Fill synchronization channels
	ReadIPFromFile(syncIP, filename);

	//connect to all other nodes
	PrepareConnections(context, reqServer, serversIP);
	
	//connect to all synchronization channels
	PrepareConnections(context, syncServer, syncIP);

	memset(polynomials, 0, sizeof(polynomials[0][0]) * CONFIDENCE_PARAM * badPlayers);
	memset(polyEvals, 0, sizeof(polyEvals[0][0]) * numOfNodes * CONFIDENCE_PARAM);
}