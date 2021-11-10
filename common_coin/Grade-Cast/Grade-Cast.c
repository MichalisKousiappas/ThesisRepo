/*
 * Example with PUSH/PULL
 * This example reads ports and hosts from the file hosts.txt
 * make sure the const NUM_OF_NODES is equal or less than the records of hosts.txt
 * to run all the processes at the same time replace NUM_OF_NODES and run this:
 * 	`for i in {0..NUM_OF_NODES - 1}; do ./multiProcesses $i NUM_OF_NODES dealer> result$i.dmp & done`
 */
#include "functions.h"

int numOfNodes;
int dealer;
int tally = 1; //count yourself
int proc_id;
int badPlayers;
struct output out = {0, 0};

int main (int argc,char *argv[])
{
	proc_id = atoi(argv[1]);
	numOfNodes= atoi(argv[2]);
	dealer = atoi(argv[3]);
	badPlayers = numOfNodes/3;
	char serversIP[numOfNodes+1][256];
	struct servers reqServer[numOfNodes+1];
	char *secret = {0};
	char temp[15] = {0};

	ValidateInput(argc);

	TraceInfo("proc_id:[%d] numOfNodes:[%d] dealer:[%d] badPlayers:[%d]\n", proc_id, numOfNodes, dealer, badPlayers);
	void *context = zmq_ctx_new();

	// Initialize
	init(serversIP);

	// dirty solution to fix the server
	char *dummy = strtok(serversIP[proc_id], ":");
	dummy = strtok( NULL, ":");
	dummy = strtok( NULL, ":");

	// build the connection for the servert correctly
	sprintf(serversIP[proc_id], "tcp://*:%s", dummy);

	TraceDebug("%d: %s\n", proc_id, serversIP[proc_id]);
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
		memcpy(temp, "110011011", sizeof("110011011"));
		secret = temp;
		DealerDistribute(reqServer, secret);
	//	Distribute(reqServer, "OK");
	}
	else
	{
		secret = GetFromDealer(reqServer);

	//	zmq_recv(reqServer[proc_id].value, temp, 10, 0);
	//	TraceInfo("Received dummy data[%d]: [%s]\n", proc_id, temp);
	}

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