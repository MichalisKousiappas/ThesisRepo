/*
 * This example reads ports and hosts from the file hosts.txt
 * make sure the const NUM_OF_NODES is equal or less than the records of hosts.txt
 * to run all the processes at the same time use the run.sh script
 */
#include "polyfunc.h"
#include "gradeddecide.h"
#include "init.h"
#include "gradedshare.h"
#include "gradedrecover.h"
#include "vote.h"

int numOfNodes;
int dealer;
int proc_id;
int badPlayers;
struct output *outArray = NULL; // used for Graded-Cast validation
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
	struct output candidate[numOfNodes];
	struct output DecideOutput;
	int tally[numOfNodes];

	ValidateInput(argc);
	void *context = zmq_ctx_new();

	//Initialize variables
	init(context, commonChannel, serversIP, polynomials, polyEvals, RootPolynomial, EvaluatedRootPoly, RootPolynomial);

	// Begin the Graded-Share protocol
	secret = SimpleGradedShare(commonChannel, polyEvals, EvaluatedRootPoly);
	ParseSecret(secret, polyEvals, EvaluatedRootPoly);

	// Begin the Graded-Decide protocol
	DecideOutput = SimpleGradedDecide(commonChannel, polyEvals, EvaluatedRootPoly, polynomials, RootPolynomial);

	// Begin Vote protocol
	Vote(commonChannel, DecideOutput, candidate);

	// Begin Graded-Recover phase
	SimpleGradedRecover(commonChannel, EvaluatedRootPoly, candidate, tally);

	TraceInfo("total messages send: [%d]\n", messages);

	#ifdef DEBUG
		for(int i = 0; i < numOfNodes; i++)
			printf("proc_id:[%d] out.code[%d] out.value:[%d]\n", i, outArray[i].code, outArray[i].value);
	#endif

	TraceInfo("tally is [%d]\n", tally[proc_id]);

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