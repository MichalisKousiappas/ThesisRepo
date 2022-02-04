/*
 * This example reads ports and hosts from the file hosts.txt
 * make sure the const NUM_OF_NODES is equal or less than the records of hosts.txt
 * to run all the processes at the same time replace NUM_OF_NODES and run this:
 * 	`for i in {0..NUM_OF_NODES - 1}; do ./multiProcesses $i NUM_OF_NODES dealer> result$i.dmp & done`
 */
#include "polyfunc.h"
#include "gradeddecide.h"
#include "init.h"
#include "gradedshare.h"
#include "gradedrecover.h"

int numOfNodes;
int dealer;
int proc_id;
int badPlayers;
struct output *outArray = NULL; // used for Graded-Cast validation
struct output *Accept = NULL; // used for Graded-Decide validation
int messages = 0;
int maxNumberOfMessages;
int StringSecreteSize;
int PrimeCongruent;
double RootOfUnity;

int main (int argc,char *argv[])
{
	proc_id = atoi(argv[1]);
	numOfNodes = atoi(argv[2]);
	dealer = atoi(argv[3]);
	badPlayers = numOfNodes/3;
	char serversIP[numOfNodes][256];
	struct servers commonChannel[numOfNodes];
	double polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers];
	double RootPolynomial[badPlayers];
	double polyEvals[numOfNodes][numOfNodes][CONFIDENCE_PARAM];
	double EvaluatedRootPoly[numOfNodes];
	char *secret;

	ValidateInput(argc);
	void *context = zmq_ctx_new();

	//Initialize variables
	init(context, commonChannel, serversIP, polynomials, polyEvals, RootPolynomial, EvaluatedRootPoly, RootPolynomial);

	// Begin the Graded-Share protocol
	secret = SimpleGradedShare(commonChannel, polyEvals, EvaluatedRootPoly);
	ParseSecret(secret, polyEvals, EvaluatedRootPoly);

	// Begin the Graded-Decide protocol
	SimpleGradedDecide(commonChannel, polyEvals, EvaluatedRootPoly, polynomials, RootPolynomial);

	// Begin Graded-Recover phase
	SimpleGradedRecover(commonChannel, EvaluatedRootPoly);

	TraceInfo("total messages send: [%d] Accept.code[%d]\n", messages, Accept[proc_id].code);

	for(int i = 0; i < numOfNodes; i++)
		printf("proc_id:[%d] out.code[%d] out.value:[%d]\n", i, outArray[i].code, outArray[i].value);

	// clean up your mess when you are done
	for(int i = 0; i < numOfNodes; i++)
	{
		TraceDebug("Closed connection[%d]\n", i);
		zmq_close(commonChannel[i].value);
	}
	free(outArray);
	zmq_ctx_destroy(context);
	TraceInfo("finished\n");
	return 0;
}