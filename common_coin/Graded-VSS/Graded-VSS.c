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
#include "dealerfunc.h"

int numOfNodes;
int dealer;
int proc_id;
int badPlayers;
struct output *outArray = NULL;
int messages = 0;
int maxNumberOfMessages;
int StringSecreteSize;
int PrimeCongruent;
int RootOfUnity;

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
	int polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers];
	int RootPolynomial[badPlayers];
	int polyEvals[numOfNodes][numOfNodes][CONFIDENCE_PARAM];
	int EvaluatedRootPoly[numOfNodes];
	char *secret;

	ValidateInput(argc);
	void *context = zmq_ctx_new();

	//Initialize variables
	init(context, commonChannel, distributorChannel, serversIP, syncIP, polynomials, polyEvals, RootPolynomial, EvaluatedRootPoly);
	TraceInfo("proc_id:[%d] numOfNodes:[%d] dealer:[%d] badPlayers:[%d]\n\t\t\t\t\t\t\t\t\t\t MaxMessages:[%d] secreteSize:[%d] primeCongruent[%d] RootOfUnity[%d]\n", 
			   proc_id, numOfNodes, dealer, badPlayers, maxNumberOfMessages, StringSecreteSize, PrimeCongruent, RootOfUnity);

	if (IsDealer)
	{
		GenerateRandomPoly(badPlayers, polynomials, RootPolynomial);
		printPolynomials(badPlayers, polynomials, RootPolynomial);
		evaluatePolynomials(badPlayers, polynomials, polyEvals, RootPolynomial, EvaluatedRootPoly);
		printEvaluatedPolys(numOfNodes, polyEvals, EvaluatedRootPoly);
	}

	secret = SimpleGradedShare(distributorChannel, polyEvals, EvaluatedRootPoly);

	if (proc_id != dealer)
	{
		ParseSecret(secret, polyEvals, EvaluatedRootPoly);
		printEvaluatedPolys(numOfNodes, polyEvals, EvaluatedRootPoly);
	}

	SimpleGradedDecide(commonChannel, distributorChannel, polyEvals, EvaluatedRootPoly, polynomials, RootPolynomial);

	TraceInfo("total messages send: [%d]\n", messages);

	for(int i = 0; i < numOfNodes; i++)
		printf("proc_id:[%d] out.code[%d] out.value:[%d]\n", i, outArray[i].code, outArray[i].value);

	// clean up your mess when you are done
	for(int i = 0; i < numOfNodes; i++)
	{
		TraceDebug("Closed connection[%d]\n", i);
		zmq_close(commonChannel[i].value);
		zmq_close(distributorChannel[i].value);
	}
	free(outArray);
	zmq_ctx_destroy(context);
	TraceInfo("finished\n");
	return 0;
}