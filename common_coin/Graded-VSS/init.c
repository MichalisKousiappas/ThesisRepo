#include "init.h"
#include "polyfunc.h"


void ReadIPFromFile(char serversIP[][256], char *filename);
void PrepareConnections(void *context, struct servers reqServer[], char serversIP[][256]);

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
		if (i == proc_id || (IsDealer && i == numOfNodes)) continue;

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

/* 
 * Utility function to check whether a number is prime or not
*/
int isPrime(int n)
{
    // Corner case
    if (n <= 1)
        return 0;
 
    if (n == 2 || n == 3)
        return 1;
 
    // Check from 2 to sqrt(n)
    for (int i = 2; i * i <= n; i++)
        if (n % i == 0)
            return 0;
 
    return 1;
}

// finding the Prime numbers
int getPrimeCongruent()
{
    int c1 = 2;
    int num1;
 
	// Printing n numbers of prime
	while (1)
	{
		// Checking the form of An+1
		num1 = (c1 * numOfNodes) + 1;
		if (isPrime(num1) && (num1 > maxNumberOfMessages)) 
			return num1;
		c1+=2;
	}
}

double RoundDouble(double var)
{
    double value = (int)(var * 10000 + .5);
    return (double)value / 10000;
}

/**
 * Initialize variables with the correct values
*/
void init(void *context, 
		struct servers reqServer[],
		char serversIP[][256],
		double polynomials[][CONFIDENCE_PARAM][badPlayers],
		gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM],
		double RootPoly[],
		gsl_complex EvaluatedRootPoly[],
		double RootPolynomial[])
{
	char filename[35] = {0};

	// Fill serversIP
	memcpy(filename, "hosts.txt", sizeof(filename));
	ReadIPFromFile(serversIP, filename);

	//connect to all other nodes
	PrepareConnections(context, reqServer, serversIP);

	for (int l = 0; l< numOfNodes; l++)
		for (int i = 0; i < CONFIDENCE_PARAM; i++)
			for (int j = 0; j < badPlayers; j++)
				polynomials[l][i][j] = 0;

	for (int l = 0; l< numOfNodes; l++)
		for (int i = 0; i < numOfNodes; i++)
			for (int j = 0; j < CONFIDENCE_PARAM; j++)
				GSL_SET_COMPLEX(&polyEvals[l][i][j], 0, 0);

	for (int i = 0; i < badPlayers; i++)
		RootPoly[i] = 0;
	
	for (int i = 0; i < numOfNodes; i++)
		GSL_SET_COMPLEX(&EvaluatedRootPoly[i], 0, 0);

	// Cheap way to make global array with variable length
	outArray = calloc(numOfNodes, sizeof(struct output));
	Accept = calloc(numOfNodes, sizeof(struct output));

	//maximume number of messages. if all processors are good
	maxNumberOfMessages = (numOfNodes * (2*numOfNodes + 1)) * 2 + numOfNodes; //as of now this is the max
	StringSecreteSize = (numOfNodes * numOfNodes * CONFIDENCE_PARAM * 2*sizeof(double));

	PrimeCongruent = getPrimeCongruent();

	srand(time(0));
	int k = rand() % numOfNodes;
	//RootOfUnity = cexp(2 * M_PI * I * k / numOfNodes);
	RootOfUnity = gsl_complex_polar(1, 2 * M_PI * k / numOfNodes);
	GSL_REAL(RootOfUnity) = fmod(GSL_REAL(RootOfUnity), PrimeCongruent);
	GSL_IMAG(RootOfUnity) = fmod(GSL_IMAG(RootOfUnity), PrimeCongruent);

/* Commented out since it might not be needed after all
	 
	 * Rout must be negative otherwise GSL interpolation won't work.
	 * the reason is that GSL can't calculate for a point outside of the graph points.
	 * if we don't have a negative root then we don't cross point 0 thus we can't solve F(0)
	
	if (RootOfUnity > 0)
		RootOfUnity = (RootOfUnity*-1) + 0.0001;

	// Round the root to 4 decimals
	//RoundDouble(RootOfUnity);

*/
	if (IsDealer)
	{
		GenerateRandomPoly(badPlayers, polynomials, RootPolynomial);
		printPolynomials(badPlayers, polynomials, RootPolynomial);
		evaluatePolynomials(badPlayers, polynomials, polyEvals, RootPolynomial, EvaluatedRootPoly);
		printEvaluatedPolys(numOfNodes, polyEvals, EvaluatedRootPoly);
	}

	TraceInfo("proc_id:[%d] numOfNodes:[%d] dealer:[%d] badPlayers:[%d]\n\t\t\t\t\t\t\t\t\t\t MaxMessages:[%d] secreteSize:[%d] primeCongruent[%d] RootOfUnity[%f%+fi]\n", 
			   proc_id, numOfNodes, dealer, badPlayers, maxNumberOfMessages, StringSecreteSize, PrimeCongruent, GSL_REAL(RootOfUnity), GSL_IMAG(RootOfUnity));
}