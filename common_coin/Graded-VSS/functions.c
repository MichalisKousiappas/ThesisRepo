#include "functions.h"
#include "polyfunc.h"

//Local Function Declarations
struct output ValidateTally(int tally);
int CountSameMessage(struct servers reqServer[], const char *commonString);
char *GetQueryBits(int node, int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int QueryBitsArray[]);
void ParseQueryBitsMessage(char *message, int array[][CONFIDENCE_PARAM]);
void PrepaireNewPolynomials(struct servers syncServer[],
						int QueryBitsArray[numOfNodes][CONFIDENCE_PARAM],
						int NewPolynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						int polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						int RootPolynomial[badPlayers]);
char *BuildMessage(int node, int NewPolynomials[][CONFIDENCE_PARAM][badPlayers]);
void ParseMessage(int node, char *message, int NewPolynomials[][CONFIDENCE_PARAM][badPlayers]);
void PrintQueryBits(int QueryBitsArray[numOfNodes][CONFIDENCE_PARAM]);
int CheckForGoodPiece(int NewPolynomials[][CONFIDENCE_PARAM][badPlayers], 
						int QueryBitsArray[][CONFIDENCE_PARAM],
						int polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						int EvaluatedRootPoly[],
						int RootPolynomial[badPlayers]);

/**
  Send the same message to all other nodes
 */
void Distribute(struct servers reqServer[], const char *commonString)
{
	char sendBuffer[StringSecreteSize];

	memset(sendBuffer, 0, sizeof(sendBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);

	sprintf(sendBuffer, "%s", commonString);

	// if ((proc_id != 0) && (proc_id != dealer) && (proc_id%3 == 0))
	// {
	// 	sprintf(sendBuffer, "%d%s", proc_id, "0011011");
	// 	TraceDebug("%s*I am a traitor hahaha[%d]\n", __FUNCTION__, proc_id);
	// }

	messages++;
	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		
		TraceDebug("Sending data as client[%d] to [%d]: [%s]\n", proc_id, i, sendBuffer);
		zmq_send(reqServer[i].value, sendBuffer, StringSecreteSize, 0);
		
		if (memcmp(commonString, "OK", 2) && commonString[0])
			messages++;
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}

/**
 * Get Message from other nodes and count the tally
*/
 int CountSameMessage(struct servers reqServer[], const char *commonString)
{
	char recvBuffer[StringSecreteSize];
	int res = 0;

	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
	
		zmq_recv(reqServer[proc_id].value, recvBuffer, StringSecreteSize, 0);
		TraceDebug("Received data as server[%d]: [%s]\n", proc_id, recvBuffer);

		// Count the messages that match yours
		if(recvBuffer[0] && !memcmp(commonString, recvBuffer, strlen(recvBuffer)))
		{
			res++;
		}
		memset(recvBuffer, 0, sizeof(recvBuffer));
	}
	TraceInfo("%s*exit[%d]\n", __FUNCTION__, res);
	return res;
}

/*
   Graded-Cast tally validation
 */
struct output ValidateTally(int tally)
{
	TraceInfo("%s*tally:[%d]\n", __FUNCTION__, tally);
	struct output out;

	if (tally >= (2*badPlayers + 1))
	{
		out.value = tally;
		out.code = 2;
	}
	else if ((tally <= (2*badPlayers)) && (tally > badPlayers))
	{
		out.value = tally;
		out.code = 1;
	}
	else
	{
		out.value = 0;
		out.code = 0;
	}

	return out;
}

/**
 * Get distributors secret 
 * distributor is the node that initiates Grade-Cast or the dealer
*/
char *GetFromDistributor(struct servers reqServer[], int distributor)
{
	TraceInfo("%s*enter\n", __FUNCTION__);
	char *result = (char*) malloc(StringSecreteSize + 1);
	char sendBuffer[5];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(result, 0, sizeof(StringSecreteSize));

	sprintf(sendBuffer, "%d", proc_id);

	TraceDebug("Sending data as client[%d] to dealer: [%s]\n", proc_id, sendBuffer);
	zmq_send(reqServer[distributor].value, sendBuffer, 5, 0);
//	TraceDebug("awaken\n");

	zmq_recv(reqServer[proc_id].value, result, StringSecreteSize, 0);
	TraceDebug("Received secret from dealer: [%s]\n", result);

	TraceInfo("%s*exit\n", __FUNCTION__);
	return result;
}

/**
  Send the same message to all other nodes
 */
void DistributorDistribute(struct servers reqServer[], const char *secret, int distributor)
{
	char sendBuffer[StringSecreteSize];
	char recvBuffer[5];
	int requestor;

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);
	// "Secret" binary string
	sprintf(sendBuffer, "%s", secret);

	messages++;
	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;
		zmq_recv(reqServer[distributor].value, recvBuffer, 5, 0);
		TraceDebug("Received data as distributor: [%s]\n", recvBuffer);
		requestor = atoi(recvBuffer);

		zmq_send(reqServer[requestor].value, sendBuffer, StringSecreteSize, 0);
		TraceDebug("Send data as distributor to [%d]: [%s]\n", requestor, sendBuffer);

		memset(recvBuffer, 0, sizeof(recvBuffer));
		messages++;
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}

/**
 * Grade-Cast for Graded-VSS
*/
char *GradeCast(struct servers reqServer[], struct servers syncServer[], int distributor, const char *message)
{
	char *commonString = {0};
	char *result = (char*) malloc(StringSecreteSize);
	int tally = 0;
	int messagesCount = 0;

	TraceInfo("%s*enter\n", __FUNCTION__);

	if (proc_id == distributor)
	{
		DistributorDistribute(syncServer, message, distributor);
		TraceDebug("%s*distirbutor:[%d] finished. Sending OK signal\n", __FUNCTION__, distributor);
		memcpy(result, message, StringSecreteSize);
		commonString = result;
		Distribute(syncServer, "OK");
	}
	else
	{
		commonString = GetFromDistributor(syncServer, distributor);
		WaitForDealerSignal(syncServer);
	}

	//Distribute the common string
	Distribute(reqServer, commonString);

	// Read the note section on README.md to understand the potential problem
	messagesCount = CountSameMessage(reqServer, commonString);
	if (messagesCount < (numOfNodes - badPlayers))
	{
		TraceDebug("%s*[%d] sending empty string\n", __FUNCTION__, proc_id);
		commonString = "";
	}

	Distribute(reqServer, commonString);
	tally = CountSameMessage(reqServer, commonString);

	outArray[distributor] = ValidateTally(tally + 1); // Count yourself when validating the tally

	memcpy(result, commonString, strlen(commonString));
	TraceInfo("%s*exit*distributor[%d] output:code[%d] value:[%d]\n", __FUNCTION__, distributor, outArray[distributor].code, outArray[distributor].value);
	
	return result;
}

/**
 * Start the Simple Graded - Decide phase
 */
void SimpleGradedDecide(struct servers reqServer[],
						struct servers syncServer[],
						int polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						int EvaluatedRootPoly[],
						int polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						int RootPolynomial[badPlayers])
{
	TraceInfo("%s*enter\n", __FUNCTION__);
	
	int QueryBitsArray[numOfNodes][CONFIDENCE_PARAM];
	int NewPolynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers];
	int GoodPieceMessages = 0, PassableMessages = 0;
	char *GradedCastMessage;
	char DecideMessage[10] = {0};

	for (int i = 0; i < numOfNodes; i++)
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
			QueryBitsArray[i][j] = 0;

	printf("-------------------GradeCast phase 1----------------------------\n");
	// all processes take turn and distribute their "secret"
	for (int distributor = 0; distributor < numOfNodes; distributor++)
	{
		GradedCastMessage = GradeCast(reqServer, syncServer, distributor, GetQueryBits(distributor, polyEvals, QueryBitsArray[proc_id]));
		printf("----------------------------------------\n");

		if (outArray[distributor].code > 0)
			ParseQueryBitsMessage(GradedCastMessage, QueryBitsArray);
	}

	// Dealer will create the new polynomials while each node waits for the dealer to finish
	PrepaireNewPolynomials(syncServer, QueryBitsArray, NewPolynomials, polynomials, RootPolynomial);

	printf("-------------------GradeCast phase 2----------------------------\n");
	// Dealer now sends out the new polynomials for each node
	for (int procNum = 0; procNum < numOfNodes; procNum++)
	{
		GradedCastMessage = GradeCast(reqServer, syncServer, dealer, BuildMessage(procNum, NewPolynomials));
		printf("----------------------------------------\n");

		if (outArray[dealer].code > 0 && !IsDealer)
			ParseMessage(procNum, GradedCastMessage, NewPolynomials);
	}

	if (CheckForGoodPiece(NewPolynomials, QueryBitsArray, polyEvals, EvaluatedRootPoly, RootPolynomial))
		sprintf(DecideMessage, "GoodPiece");
	else
		memset(DecideMessage, 0, sizeof(DecideMessage));

	Distribute(reqServer, DecideMessage);
	GoodPieceMessages = CountSameMessage(reqServer, "GoodPiece");

	if (GoodPieceMessages >= (numOfNodes - badPlayers))
		sprintf(DecideMessage, "Passable");
	else
		memset(DecideMessage, 0, sizeof(DecideMessage));

	Distribute(reqServer, DecideMessage);
	PassableMessages = CountSameMessage(reqServer, "Passable");
	Accept[proc_id] = ValidateTally(PassableMessages + 1);

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, PassableMessages);
}

/**
 * Parse the secret received from dealer.
 */
void ParseSecret(char *secret, int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int EvaluatedRootPoly[])
{
	// Dealer process does not need to parse the secret
	if (IsDealer)
		return;
	
	char* token = strtok(secret, SECRETE_DELIMITER);
	EvaluatedRootPoly[proc_id] = atoi(token);

	for (int i = 0; i < numOfNodes; i++)
	{
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
		{
			token = strtok(0, SECRETE_DELIMITER);
			polyEvals[proc_id][i][j] = atoi(token);
		}
	}
	
	// When debugging is on, printf the parsed message
	printEvaluatedPolys(numOfNodes, polyEvals, EvaluatedRootPoly);	
}

/**
 * Random Query bits means check if bit 2 & 3 for example are set which is just 
 * more complicated than just substracting a random number from all your numbers
 */
char *GetQueryBits(int node, int polyEvals[][numOfNodes][CONFIDENCE_PARAM], int QueryBitsArray[])
{
	TraceInfo("%s*enter\n", __FUNCTION__);
	int length = 0;
	int randomNum = rand() % (MAX_COEFICIENT/2);

	if (node != proc_id)
	{
		TraceDebug("%s*exit*not my turn yet\n", __FUNCTION__);
		return "";
	}

	char *result = (char*) malloc(StringSecreteSize);
	memset(result, 0, sizeof(StringSecreteSize)-1);

	length += snprintf(result+length , StringSecreteSize-length, "%d%s", proc_id, SECRETE_DELIMITER);

	for(int i = 0; i < CONFIDENCE_PARAM; i++)
	{
		QueryBitsArray[i] = polyEvals[proc_id][proc_id][i] - randomNum;
		length += snprintf(result+length , StringSecreteSize-length, "%d%s", QueryBitsArray[i], SECRETE_DELIMITER);
	}

	//Close the close so parsing can be done correctly
	length += snprintf(result+length , StringSecreteSize-length, "%s", SECRETE_DELIMITER);	
	result[length-1] = '\0';

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, length);
	return result;
}

/**
 * Parse the secret received from dealer and store it into array.
 * The secret are the evaulation of the polyonims
 */
void ParseQueryBitsMessage(char *message, int array[][CONFIDENCE_PARAM])
{
	char* token = strtok(message, SECRETE_DELIMITER);
	int Process_id = atoi(token);

	for (int j = 0; j < CONFIDENCE_PARAM; j++)
	{
		token = strtok(0, SECRETE_DELIMITER);
		array[Process_id][j] = atoi(token);
	}
}

/**
 * All processes wait for dealer to prepaire the new polynomials that will be used
 * to check if the secrete is passable or not
*/
void PrepaireNewPolynomials(struct servers syncServer[],
						int QueryBitsArray[numOfNodes][CONFIDENCE_PARAM],
						int NewPolynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						int polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						int RootPolynomial[badPlayers])
{
	TraceInfo("%s*enter\n", __FUNCTION__);
	
	// Print the Query bits you have when debugging info is on
	PrintQueryBits(QueryBitsArray);

	if (IsDealer)
	{
		for (int k = 0; k < numOfNodes; k++)
		{
			for (int i = 0; i < CONFIDENCE_PARAM; i++)
			{
				for (int j = 0; j < badPlayers; j++)
				{
					NewPolynomials[k][i][j] = polynomials[k][i][j] + QueryBitsArray[k][i] * RootPolynomial[j];
				}
			}
		}
		TraceDebug("%s*NEW POLYNOMIALS\n", __FUNCTION__);
		printPolynomials(badPlayers, NewPolynomials, RootPolynomial);
		Distribute(syncServer, "OK");
	}
	else
	{
		WaitForDealerSignal(syncServer);
	}
	
	TraceInfo("%s*exit\n", __FUNCTION__);
}

/**
 * Builds the new polynomials to be send.
*/
char *BuildMessage(int node, int NewPolynomials[][CONFIDENCE_PARAM][badPlayers])
{
	TraceInfo("%s*enter\n", __FUNCTION__);
	int length = 0;
	char *result = (char*) malloc(StringSecreteSize);
	memset(result, 0, sizeof(StringSecreteSize)-1);

	if (!IsDealer)
	{
		TraceDebug("%s*exit*Not my job\n", __FUNCTION__);
		return "";
	}

	length += snprintf(result+length , StringSecreteSize-length, "%d%s", node, SECRETE_DELIMITER);

	for (int j = 0; j < CONFIDENCE_PARAM; j++)
	{
		for(int i = 0; i < badPlayers; i++)
		{
			length += snprintf(result+length , StringSecreteSize-length, "%d%s", NewPolynomials[node][j][i], SECRETE_DELIMITER);
		}
	}

	//Close the close so parsing can be done correctly
	length += snprintf(result+length , StringSecreteSize-length, "%s", SECRETE_DELIMITER);

	result[length-1] = '\0';

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, length);
	return result;
}

/**
 * Parse the message received from dealer in step 2.
 */
void ParseMessage(int node, char *message, int NewPolynomials[][CONFIDENCE_PARAM][badPlayers])
{	
	TraceInfo("%s*enter\n", __FUNCTION__);
	
	char* token = strtok(message, SECRETE_DELIMITER);
	TraceDebug("%s*token[%d]\n", __FUNCTION__, atoi(token));

	for (int i = 0; i < CONFIDENCE_PARAM; i++)
	{
		for (int j = 0; j < badPlayers; j++)
		{
			token = strtok(0, SECRETE_DELIMITER);
			NewPolynomials[node][i][j] = atoi(token);
		}
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}

void PrintQueryBits(int QueryBitsArray[numOfNodes][CONFIDENCE_PARAM])
{
	#ifndef DEBUG
		return;
	#endif

	printf("\nPrinting Query bits\n");
	for (int i = 0; i < numOfNodes; i++)
	{
		printf("\nnode: %d\n", i);
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
		{
			printf("[%d] ", QueryBitsArray[i][j]);
		}
	}
	printf("\n");	
}

int CheckForGoodPiece(int NewPolynomials[][CONFIDENCE_PARAM][badPlayers], 
						int QueryBitsArray[][CONFIDENCE_PARAM],
						int polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						int EvaluatedRootPoly[],
						int RootPolynomial[badPlayers])
{
	TraceInfo("%s*enter\n", __FUNCTION__);

	int Pij, TplusQmultiS;
	int counter1 = 0, counter2 = 0;
	int res;

	printPolynomials(badPlayers, NewPolynomials, RootPolynomial);

	if (IsDealer)
		return 1;

	for (int i = 0; i < numOfNodes; i++)
	{
		if (outArray[i].code == 0)
			continue;
		
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
		{
			counter1++;
			Pij = poly_eval(NewPolynomials[i][j], badPlayers, pow(RootOfUnity, proc_id));
			TplusQmultiS = polyEvals[proc_id][i][j] + QueryBitsArray[i][j] * EvaluatedRootPoly[proc_id];
			TraceDebug("i:[%d] j:[%d] Pij:[%d] TplusQmulitS:[%d] Qbit:[%d] RootPoly:[%d]\n", i, j, Pij, TplusQmultiS, QueryBitsArray[i][j], EvaluatedRootPoly[proc_id]);
			if (Pij == TplusQmultiS)
			{
				counter2++;
			}
		}
		TraceDebug("\n");
	}
	res = (counter1 == counter2);

	if (res)
		RootPolynomial[proc_id] = EvaluatedRootPoly[proc_id];
	else
		RootPolynomial[proc_id] = 0;

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, res);
	return res;
}