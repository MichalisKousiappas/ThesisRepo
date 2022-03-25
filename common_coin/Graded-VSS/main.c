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
char TimeVar[25];
int CONFIDENCE_PARAM;

int main (int argc,char *argv[])
{
	proc_id = atoi(argv[1]);
	numOfNodes = atoi(argv[2]);
	badPlayers = numOfNodes/3;
	CONFIDENCE_PARAM = (int) log(10 * numOfNodes * numOfNodes); //set here since constant is needed right at the start
	char serversIP[numOfNodes][256];
	struct servers commonChannel[numOfNodes];
	double polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers];
	double RootPolynomial[badPlayers];
	double polyEvals[numOfNodes][numOfNodes][CONFIDENCE_PARAM];
	double EvaluatedRootPoly[numOfNodes];
	double Secret_hj[numOfNodes][numOfNodes];
	char *secret;
	struct output candidate[numOfNodes];
	struct output DecideOutput[numOfNodes][numOfNodes]; //rows are the process, columns are the dealer
	int tally[numOfNodes];

	ValidateInput(argc);
	void *context = zmq_ctx_new();

	//Initialize variables
	init(context, commonChannel, serversIP, polynomials, polyEvals, RootPolynomial, EvaluatedRootPoly, RootPolynomial, Secret_hj);

	for (dealer = 0; dealer < numOfNodes; dealer++)
	{
		if (IsDealer)
		{
			TraceDebug("Generating polynomials...\n");
			GenerateRandomPoly(badPlayers, polynomials, RootPolynomial);
			printPolynomials(badPlayers, polynomials, RootPolynomial);
			printRootPolyOnly(RootPolynomial);
			evaluatePolynomials(badPlayers, polynomials, polyEvals, RootPolynomial, EvaluatedRootPoly);
			printEvaluatedPolys(numOfNodes, polyEvals, EvaluatedRootPoly);
			TraceDebug("Done\n");
		}

		// Begin the Graded-Share protocol
		secret = SimpleGradedShare(commonChannel, polyEvals, EvaluatedRootPoly);
		ParseSecret(secret, polyEvals, EvaluatedRootPoly);

		// Begin the Graded-Decide protocol
		DecideOutput[proc_id][dealer] = SimpleGradedDecide(commonChannel, polyEvals, EvaluatedRootPoly, polynomials, RootPolynomial, Secret_hj);
	}

	// Begin Vote protocol
	Vote(commonChannel, DecideOutput, candidate);

	#ifdef DEBUG
		for(int i = 0; i < numOfNodes; i++)
			printf("candidate[%d]: [%d]\n", i, candidate[i].code);
	#endif

	// Begin Graded-Recover phase
	SimpleGradedRecover(commonChannel, Secret_hj, candidate, tally);
/*
	#ifdef DEBUG
		for(int i = 0; i < numOfNodes; i++)
			for(int k = 0; k < numOfNodes; k++)
			printf("proc:[%d] dealer[%d] secret:[%f]\n", i, k, Secret_hj[i][k]);
	#endif
*/
	TraceInfo("total messages send: [%d]\n", messages);
	TraceInfo("tally is [%d]\n", tally[proc_id]);

	// clean up your mess when you are done
	for(int i = 0; i < numOfNodes; i++)
	{
		//TraceDebug("Closed connection[%d]\n", i);
		zmq_close(commonChannel[i].value);
	}
	free(outArray);
	free(secret);
	zmq_ctx_destroy(context);
	TraceInfo("finished\n");
	return 0;
}