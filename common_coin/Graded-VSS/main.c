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

# define timersub(a, b, result)\
  do {\
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;\
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;\
    if ((result)->tv_usec < 0) {\
      --(result)->tv_sec;\
      (result)->tv_usec += 1000000;\
    }\
  } while (0)

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
char TimeVar[25] = {0};
int CONFIDENCE_PARAM;
int *TimedOut;

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
	int tally;

	struct timeval tval_before, tval_after, tval_result;

	ValidateInput(argc);
	void *context = zmq_ctx_new();

	//Initialize variables
	init(context, commonChannel, serversIP, polynomials, polyEvals, RootPolynomial, EvaluatedRootPoly, RootPolynomial, Secret_hj);

	RandomDeath();

	for (dealer = 0; dealer < numOfNodes; dealer++)
	{
		if (TimedOut[dealer] == 1)
			continue;

		if (dealer == 1)
			gettimeofday(&tval_before, NULL);

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

		RandomDeath();

		// Begin the Graded-Share protocol
		secret = SimpleGradedShare(commonChannel, polyEvals, EvaluatedRootPoly);
		ParseSecret(secret, polyEvals, EvaluatedRootPoly);

		RandomDeath();

		if (TimedOut[dealer] == 1)
			continue;

		// Begin the Graded-Decide protocol
		DecideOutput[proc_id][dealer] = SimpleGradedDecide(commonChannel, polyEvals, EvaluatedRootPoly, polynomials, RootPolynomial, Secret_hj);
		
		if (dealer == 1)
		{
			gettimeofday(&tval_after, NULL);
			timersub(&tval_after, &tval_before, &tval_result);
			printf("Time elapsed: %ld.%06ld\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
		}
	}

	RandomDeath();

	// Begin Vote protocol
	Vote(commonChannel, DecideOutput, candidate);

	RandomDeath();

	for(int i = 0; i < numOfNodes; i++)
		TraceDebug("candidate[%d]: [%d]\n", i, candidate[i].code);

	RandomDeath();

	// Begin Graded-Recover phase
	SimpleGradedRecover(commonChannel, Secret_hj, candidate, &tally);

/*
	#ifdef DEBUG
		for(int i = 0; i < numOfNodes; i++)
			for(int k = 0; k < numOfNodes; k++)
			printf("proc:[%d] dealer[%d] secret:[%f]\n", i, k, Secret_hj[i][k]);
	#endif
*/
	TraceInfo("total messages send: [%d]\n", messages);
	TraceInfo("tally is [%d]\n", tally);

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