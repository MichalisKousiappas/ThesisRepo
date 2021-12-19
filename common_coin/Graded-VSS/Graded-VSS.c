/*
 * Example with PUSH/PULL
 * This example reads ports and hosts from the file hosts.txt
 * make sure the const NUM_OF_NODES is equal or less than the records of hosts.txt
 * to run all the processes at the same time replace NUM_OF_NODES and run this:
 * 	`for i in {0..NUM_OF_NODES - 1}; do ./multiProcesses $i NUM_OF_NODES dealer> result$i.dmp & done`
 */
#include "polyfunc.h"
#include "functions.h"
#include "init.h"

int numOfNodes;
int dealer;
int tally = 1; //count yourself
int proc_id;
int badPlayers;
struct output out = {0, 0};

int main (int argc,char *argv[])
{
	proc_id = atoi(argv[1]);
	numOfNodes = atoi(argv[2]);
	dealer = atoi(argv[3]);
	badPlayers = numOfNodes/3;
	char serversIP[numOfNodes][256];
	char syncIP[numOfNodes][256];
	struct servers commonChannel[numOfNodes];
	struct servers distributorChannel[numOfNodes];
	double polynomials[CONFIDENCE_PARAM][badPlayers];
	double polyEvals[numOfNodes][CONFIDENCE_PARAM];

	ValidateInput(argc);

	TraceInfo("proc_id:[%d] numOfNodes:[%d] dealer:[%d] badPlayers:[%d]\n", proc_id, numOfNodes, dealer, badPlayers);
	void *context = zmq_ctx_new();

	//Initialize variables
	init(context, commonChannel, distributorChannel, serversIP, syncIP);

	memset(polynomials, 0, sizeof(polynomials[0][0]) * CONFIDENCE_PARAM * badPlayers);
	memset(polyEvals, 0, sizeof(polyEvals[0][0]) * numOfNodes * CONFIDENCE_PARAM);

	if (proc_id == dealer)
	{
		GenerateRandomPoly(badPlayers, polynomials);

		#ifdef DEBUG
			printPolynomials(badPlayers, polynomials);
		#endif
	
		evaluatePolynomials(badPlayers, polynomials, polyEvals);

		#ifdef DEBUG
			printEvaluatedPolys(numOfNodes, polyEvals);
		#endif
	}

	exit(0);

	//all processes take turn and distribute their "secret"
	for (int distributor = 0; distributor < numOfNodes; distributor++)
		GradeCast(commonChannel, distributorChannel, distributor);

	// clean up your mess when you are done
	for(int i = 0; i < numOfNodes; i++)
	{
		TraceDebug("Closed connection[%d]\n", i);
		zmq_close(commonChannel[i].value);
		zmq_close(distributorChannel[i].value);
	}
	zmq_ctx_destroy(context);
	TraceInfo("finished\n");
	return 0;
}