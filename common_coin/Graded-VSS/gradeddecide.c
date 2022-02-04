#include "gradeddecide.h"
#include "polyfunc.h"
#include <gsl/gsl_spline.h>
#include <gsl/gsl_errno.h>

//Local Function Declarations
struct output ValidateTally(int tally);
int CountSameMessage(struct servers reqServer[], const char *message);
char *GetQueryBits(int node, double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double QueryBitsArray[]);
int ParseQueryBitsMessage(char *message, double array[][CONFIDENCE_PARAM]);
void PrepaireNewPolynomials(struct servers syncServer[],
						double QueryBitsArray[numOfNodes][CONFIDENCE_PARAM],
						double NewPolynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						double polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						double RootPolynomial[badPlayers]);
char *BuildMessage(int node, double NewPolynomials[][CONFIDENCE_PARAM][badPlayers]);
int ParseMessage(int node, char *message, double NewPolynomials[][CONFIDENCE_PARAM][badPlayers]);
void PrintQueryBits(double QueryBitsArray[numOfNodes][CONFIDENCE_PARAM]);
int CheckForGoodPiece(double NewPolynomials[][CONFIDENCE_PARAM][badPlayers],
						double QueryBitsArray[][CONFIDENCE_PARAM],
						double polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						double EvaluatedRootPoly[],
						double RootPolynomial[badPlayers]);
void Traitor(char *sendBuffer);
int ParsePiece(char *Piece, double RootPolynomial[]);
void GetPieces(struct servers reqServer[], double RootPolynomial[]);
double CalculatePolynomial(double EvaluatedRootPoly[]);
char *GradeCastPhaseA(struct servers reqServer[], int distributor, const char *message);
int CountSameMessageAgain(struct servers reqServer[], const char *message, int check);


/**
  Send the same message to all other nodes -- Doesn't wait for an answer
 */
void Distribute(struct servers reqServer[], const char *commonString)
{
	char sendBuffer[StringSecreteSize];

	memset(sendBuffer, 0, sizeof(sendBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);

	sprintf(sendBuffer, "%s", commonString);

	if (memcmp(commonString, "OK", 2) && commonString[0])
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
	char sendBuffer[56];

	memset(sendBuffer, 0, sizeof(sendBuffer));
	memset(result, 0, sizeof(StringSecreteSize));

	sprintf(sendBuffer, "%d %s", proc_id, "OK");

	zmq_recv(reqServer[proc_id].value, result, StringSecreteSize, 0);
	TraceDebug("Received secret from distributor: [%s]\n", result);

	TraceDebug("Sending data as client[%d] to distributor[%d]: [%s]\n", proc_id, distributor, sendBuffer);
	zmq_send(reqServer[distributor].value, sendBuffer, strlen(sendBuffer), 0);

	TraceInfo("%s*exit\n", __FUNCTION__);
	return result;
}

/**
  Send the same message to all other nodes
 */
void DistributorDistribute(struct servers reqServer[], const char *secret, int distributor)
{
	char sendBuffer[StringSecreteSize];
	char recvBuffer[56];

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

		TraceDebug("Sending data as distributor to [%d]: [%s]\n", i, sendBuffer);
		zmq_send(reqServer[i].value, sendBuffer, StringSecreteSize, 0);

		zmq_recv(reqServer[distributor].value, recvBuffer, sizeof(recvBuffer) - 1, 0);
		TraceDebug("Received confirmation as distributor[%d]: [%s]\n", distributor, recvBuffer);

		memset(recvBuffer, 0, sizeof(recvBuffer));
		messages++;
	}
	TraceInfo("%s*exit\n", __FUNCTION__);
}

/**
 * Grade-Cast Phase A
 * In phase A, the distributor will send his message to all other processes.
*/
char *GradeCastPhaseA(struct servers reqServer[], int distributor, const char *message)
{
	char *result = (char*) malloc(StringSecreteSize);
	char *commonString = {0};

	memset(result, 0, StringSecreteSize);
	TraceInfo("%s*enter\n", __FUNCTION__);

	if (proc_id == distributor)
	{
		DistributorDistribute(reqServer, message, distributor);
		TraceDebug("%s*distirbutor:[%d] finished. Sending OK signal\n", __FUNCTION__, distributor);
		memcpy(result, message, StringSecreteSize);
		commonString = result;
		Distribute(reqServer, "OK");
	}
	else
	{
		commonString = GetFromDistributor(reqServer, distributor);
		WaitForDealerSignal(reqServer);
	}

	TraceInfo("%s*exit\n", __FUNCTION__);
	memcpy(result, commonString, strlen(commonString));
	return result;
}

/**
 * CountSameMessage
 * All processes take turn and send their message to all other nodes.
 * if its not their turn to send they wait for the distributor processes to send.
*/
int CountSameMessage(struct servers reqServer[], const char *message)
{
	int messagesCount = 1;
	char *StringZ;

	TraceInfo("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < numOfNodes; i++)
	{
		if (proc_id == i)
		{
			DistributorDistribute(reqServer, message, i);
			TraceDebug("%s*Process:[%d] finished. Sending OK signal\n", __FUNCTION__, i);
			Distribute(reqServer, "OK");
		}
		else
		{
			StringZ = GetFromDistributor(reqServer, i);

			if(StringZ[0] && !memcmp(message, StringZ, strlen(StringZ)))
			{
				messagesCount++;
			}
			WaitForDealerSignal(reqServer);
		}
	}

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, messagesCount);
	return messagesCount;
}

/**
 * CountSameMessageAgain
 * If in phase B the common message was received enough times then
 * 	in phase C redistribute it, else send empty string
*/
int CountSameMessageAgain(struct servers reqServer[], const char *message, int check)
{
	int tally = 1;
	char *StringZ;

	TraceInfo("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < numOfNodes; i++)
	{
		if (proc_id == i)
		{
			DistributorDistribute(reqServer, check ? message : "", i);
			TraceDebug("%s*Process:[%d] finished. Sending OK signal\n", __FUNCTION__, i);
			Distribute(reqServer, "OK");
		}
		else
		{
			StringZ = GetFromDistributor(reqServer, i);

			if(StringZ[0] && !memcmp(message, StringZ, strlen(StringZ)))
			{
				tally++;
			}
			WaitForDealerSignal(reqServer);
		}
	}

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, tally);
	return tally;
}

/**
 * Grade-Cast for Graded-VSS
*/
char *GradeCast(struct servers reqServer[], int distributor, const char *message)
{
	char *commonString = {0};
	char *result = (char*) malloc(StringSecreteSize);
	int tally = 0;
	int messagesCount = 0;

	memset(result, 0, StringSecreteSize);
	TraceInfo("%s*enter\n", __FUNCTION__);

	// Phase A
	commonString = GradeCastPhaseA(reqServer, distributor, message);

	// Phase B
	messagesCount = CountSameMessage(reqServer, commonString);

	Traitor(commonString);

	// Phase C
	if (messagesCount < (numOfNodes - badPlayers))
		tally = CountSameMessageAgain(reqServer, commonString, 0);
	else
		tally = CountSameMessageAgain(reqServer, commonString, 1);

	outArray[distributor] = ValidateTally(tally);
	TraceInfo("%s*exit*distributor[%d] output:code[%d] value:[%d]\n", __FUNCTION__, distributor, outArray[distributor].code, outArray[distributor].value);
	memcpy(result, commonString, strlen(commonString));
	return result;
}

/**
 * Start the Simple Graded - Decide phase
 */
void SimpleGradedDecide(struct servers reqServer[],
						double polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						double EvaluatedRootPoly[],
						double polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						double RootPolynomial[badPlayers])
{
	TraceInfo("%s*enter\n", __FUNCTION__);

	double QueryBitsArray[numOfNodes][CONFIDENCE_PARAM];
	double NewPolynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers];
	int GoodPieceMessages = 0, PassableMessages = 0;
	char *GradedCastMessage;
	char DecisionMessage[10] = {0};
	struct output DealersOutput;

	for (int i = 0; i < numOfNodes; i++)
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
			QueryBitsArray[i][j] = 0;

	printf("-------------------SimpleGraded Decide phase 1----------------------------\n");
	// all processes take turn and distribute their "secret"
	for (int distributor = 0; distributor < numOfNodes; distributor++)
	{
		GradedCastMessage = GradeCast(reqServer, distributor, GetQueryBits(distributor, polyEvals, QueryBitsArray[proc_id]));
		printf("----------------------------------------\n");

		if (outArray[distributor].code > 0)
		{
			// Parse QueryBits, if message seems invalid then reject the sender
			if (ParseQueryBitsMessage(GradedCastMessage, QueryBitsArray))
			{
				outArray[distributor].code = 0;
				outArray[distributor].value = 0;
			}
		}
	}

	// Save the dealers output when he is pretending to be a normal processes
	DealersOutput = outArray[dealer];

	// Dealer will create the new polynomials while each node waits for the dealer to finish
	PrepaireNewPolynomials(reqServer, QueryBitsArray, NewPolynomials, polynomials, RootPolynomial);

	printf("-------------------SimpleGraded Decide phase 2----------------------------\n");
	// Dealer now sends out the new polynomials for each node
	for (int procNum = 0; procNum < numOfNodes; procNum++)
	{
		GradedCastMessage = GradeCast(reqServer, dealer, BuildMessage(procNum, NewPolynomials));
		printf("----------------------------------------\n");

		if (outArray[dealer].code > 0 && !IsDealer)
			ParseMessage(procNum, GradedCastMessage, NewPolynomials);
	}

	// Dealers outArray was overwritten from before. return it to its original state
	outArray[dealer] = DealersOutput;

	if (CheckForGoodPiece(NewPolynomials, QueryBitsArray, polyEvals, EvaluatedRootPoly, RootPolynomial))
		sprintf(DecisionMessage, "GoodPiece");
	else
		memset(DecisionMessage, 0, sizeof(DecisionMessage));

	GoodPieceMessages = CountSameMessage(reqServer, DecisionMessage);

	if (GoodPieceMessages < (numOfNodes - badPlayers))
		PassableMessages = CountSameMessageAgain(reqServer, "Passable", 0);
	else
		PassableMessages = CountSameMessageAgain(reqServer, "Passable", 1);

	Accept[proc_id] = ValidateTally(PassableMessages);
	TraceInfo("%s*exit[%d]\n", __FUNCTION__, PassableMessages);
}

/**
 * Parse the secret received from dealer.
 */
int ParseSecret(char *secret, double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double EvaluatedRootPoly[])
{
	// Dealer process does not need to parse the secret
	if (IsDealer)
		return 0;

	TraceInfo("%s*enter\n", __FUNCTION__);

	if (secret[strlen(secret) - 1] != '|')
	{
		TraceInfo("%s*exit*Invalid Secret\n", __FUNCTION__);
		return 1;
	}

	char* token = strtok(secret, MESSAGE_DELIMITER);
	EvaluatedRootPoly[proc_id] = strtod(token, NULL);

	for (int i = 0; i < numOfNodes; i++)
	{
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
		{
			token = strtok(0, MESSAGE_DELIMITER);
			if (token != NULL)
				polyEvals[proc_id][i][j] = strtod(token, NULL);
			else
			{
				TraceInfo("%s*exit*Invalid Secret[%d][%d]\n", __FUNCTION__,i, j);
				return 1;
			}
		}
	}

	// When debugging is on, printf the parsed message
	printEvaluatedPolys(numOfNodes, polyEvals, EvaluatedRootPoly);
	TraceInfo("%s*exit\n", __FUNCTION__);
	return 0;
}

/**
 * Random Query bits means check if bit 2 & 3 for example are set which is just
 * more complicated than just substracting a random number from all your numbers
 */
char *GetQueryBits(int node, double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double QueryBitsArray[])
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

	length += snprintf(result+length , StringSecreteSize-length, "%d%s", proc_id, MESSAGE_DELIMITER);

	for(int i = 0; i < CONFIDENCE_PARAM; i++)
	{
		QueryBitsArray[i] = polyEvals[proc_id][proc_id][i] - randomNum;
		length += snprintf(result+length , StringSecreteSize-length, "%f%s", QueryBitsArray[i], MESSAGE_DELIMITER);
	}

	Traitor(result);

	//Close the close so parsing can be done correctly
	length += snprintf(result+length , StringSecreteSize-length, "%s", MESSAGE_DELIMITER);
	result[length-1] = '\0';

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, length);
	return result;
}

/**
 * Parse the secret received from dealer and store it into array.
 * The secret are the evaulation of the polyonims
 */
int ParseQueryBitsMessage(char *message, double array[][CONFIDENCE_PARAM])
{
	printf("ParseQueryBitsMessage [%s] size:[%ld]\n", message, strlen(message));
	if (message[strlen(message) - 1] != '|')
	{
		TraceInfo("%s*exit*Invalid Query Bits message\n", __FUNCTION__);
		return 1;
	}

	char* token = strtok(message, MESSAGE_DELIMITER);
	int Process_id = atoi(token);

	for (int j = 0; j < CONFIDENCE_PARAM; j++)
	{
		token = strtok(0, MESSAGE_DELIMITER);
		if (token != NULL)
			array[Process_id][j] = strtod(token, NULL);
		else
		{
			TraceInfo("%s*exit*Invalid Query Bits[%d]\n", __FUNCTION__, j);
			return 1;
		}
	}

	return 0;
}

/**
 * All processes wait for dealer to prepaire the new polynomials that will be used
 * to check if the secrete is passable or not
*/
void PrepaireNewPolynomials(struct servers syncServer[],
						double QueryBitsArray[numOfNodes][CONFIDENCE_PARAM],
						double NewPolynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						double polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						double RootPolynomial[badPlayers])
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
char *BuildMessage(int node, double NewPolynomials[][CONFIDENCE_PARAM][badPlayers])
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

	length += snprintf(result+length , StringSecreteSize-length, "%d%s", node, MESSAGE_DELIMITER);

	for (int j = 0; j < CONFIDENCE_PARAM; j++)
	{
		for(int i = 0; i < badPlayers; i++)
		{
			length += snprintf(result+length , StringSecreteSize-length, "%f%s", NewPolynomials[node][j][i], MESSAGE_DELIMITER);
		}
	}

	//Close the close so parsing can be done correctly
	length += snprintf(result+length , StringSecreteSize-length, "%s", MESSAGE_DELIMITER);

	result[length-1] = '\0';

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, length);
	return result;
}

/**
 * Parse the message received from dealer in step 2.
 */
int ParseMessage(int node, char *message, double NewPolynomials[][CONFIDENCE_PARAM][badPlayers])
{
	printf("ParseMessage [%s] size:[%ld]\n", message, strlen(message));
	if (message[strlen(message) - 1] != '|')
	{
		TraceInfo("%s*exit*Invalid Message\n", __FUNCTION__);
		return 1;
	}

	TraceInfo("%s*enter\n", __FUNCTION__);

	char* token = strtok(message, MESSAGE_DELIMITER);
	TraceDebug("%s*token[%d]\n", __FUNCTION__, atoi(token));

	for (int i = 0; i < CONFIDENCE_PARAM; i++)
	{
		for (int j = 0; j < badPlayers; j++)
		{
			token = strtok(0, MESSAGE_DELIMITER);
			if (token != NULL)
				NewPolynomials[node][i][j] = strtod(token, NULL);
			else
			{
				TraceInfo("%s*exit*Invalid Message[%d][%d]\n", __FUNCTION__,i, j);
				return 1;
			}
		}
	}

	TraceInfo("%s*exit\n", __FUNCTION__);
	return 0;
}

/**
 * Prints the Query bits
 */
void PrintQueryBits(double QueryBitsArray[numOfNodes][CONFIDENCE_PARAM])
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
			printf("[%f] ", QueryBitsArray[i][j]);
		}
	}
	printf("\n");
}

/**
 * Check if math checks out.
 */
int CheckForGoodPiece(double NewPolynomials[][CONFIDENCE_PARAM][badPlayers],
						double QueryBitsArray[][CONFIDENCE_PARAM],
						double polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						double EvaluatedRootPoly[],
						double RootPolynomial[badPlayers])
{
	TraceInfo("%s*enter\n", __FUNCTION__);

	double Pij, TplusQmultiS;
	int counter1 = 0, counter2 = 0;
	int res;
	int power;

	printPolynomials(badPlayers, NewPolynomials, RootPolynomial);

	if (proc_id == 0)
		power = numOfNodes;
	else
		power = proc_id;

	for (int i = 0; i < numOfNodes; i++)
	{
		if ((outArray[i].code == 0) && (i != proc_id))
			continue;

		// Enable in case you want excesive debugging.
		printf("i:[%d]\n",i);
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
		{
			counter1++;

			Pij = gsl_poly_eval(NewPolynomials[i][j], badPlayers, pow(RootOfUnity, power));
			TplusQmultiS = polyEvals[proc_id][i][j] + QueryBitsArray[i][j] * EvaluatedRootPoly[proc_id];
			TraceDebug("i:[%d] j:[%d] Pij:[%f] TplusQmulitS:[%f] Qbit:[%f] RootPoly:[%f]\n",
			i, j, Pij, TplusQmultiS, QueryBitsArray[i][j], EvaluatedRootPoly[proc_id]);

			// limit the precision check to 2 bits instead of 4. This is because the calculations can't be this precise
			if ( fabs(Pij - TplusQmultiS) <= 0.0001)
			{
				counter2++;
			}
			else
			{
				TraceDebug("Error here\n");
			}
		}
		printf("\n");
	}
	res = ((counter1 == counter2) && (counter1 != 0));

	if (!res)
		EvaluatedRootPoly[proc_id] = 0;

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, res);
	return res;
}

/**
 * Simulate traitor processes.
 */
void Traitor(char *sendBuffer)
{
	if (TRAITORS && (proc_id != 0) && (proc_id != dealer) && (proc_id%3 == 0))
	{
		sprintf(sendBuffer, "%d%s", proc_id, "0011011");
		TraceDebug("%s*I am a traitor hahaha[%d]\n", __FUNCTION__, proc_id);
	}
}


void SimpleGradedRecover(struct servers reqServer[], double EvaluatedRootPoly[])
{
	char Piece[StringSecreteSize];
	memset(Piece, 0, StringSecreteSize);
	TraceInfo("%s*enter\n", __FUNCTION__);

	sprintf(Piece, "%d%s%f%s", proc_id, MESSAGE_DELIMITER, EvaluatedRootPoly[proc_id], MESSAGE_DELIMITER);
	Distribute(reqServer, Piece);
	GetPieces(reqServer, EvaluatedRootPoly);

	for (int i = 0; i < numOfNodes; i++)
		printf("i:[%d] Si:[%f]\n", i, EvaluatedRootPoly[i]);

	double finale = CalculatePolynomial(EvaluatedRootPoly);
	TraceInfo("%s*exit*finale[%f]\n", __FUNCTION__, round(finale));
}

/**
 * Receive the pieces of Simple Graded - Recover phase
*/
 void GetPieces(struct servers reqServer[], double EvaluatedRootPoly[])
{
	char recvBuffer[StringSecreteSize + 1];
	memset(recvBuffer, 0, sizeof(recvBuffer));

	TraceInfo("%s*enter\n", __FUNCTION__);

	for (int i = 0; i < numOfNodes; i++)
	{
		if (i == proc_id) continue;

		zmq_recv(reqServer[proc_id].value, recvBuffer, StringSecreteSize, 0);
		TraceDebug("Received data as server[%d]: [%s]\n", proc_id, recvBuffer);

		ParsePiece(recvBuffer, EvaluatedRootPoly);
		memset(recvBuffer, 0, sizeof(recvBuffer));
	}

	TraceInfo("%s*exit\n", __FUNCTION__);
}

/**
 * Parse the message received from dealer in step 2.
 */
int ParsePiece(char *Piece, double EvaluatedRootPoly[])
{
	TraceInfo("%s*enter\n", __FUNCTION__);

	if (Piece[strlen(Piece) - 1] != '|')
	{
		TraceInfo("%s*exit*Invalid Piece 1\n", __FUNCTION__);
		return 1;
	}

	char* token = strtok(Piece, MESSAGE_DELIMITER);
	TraceDebug("%s*token[%d]\n", __FUNCTION__, atoi(token));
	int Process = atoi(token);

	if (outArray[Process].code == 0)
	{
		TraceInfo("%s*exit*ignoring piece\n", __FUNCTION__);
		return 1;
	}

	token = strtok(0, MESSAGE_DELIMITER);
	if (token != NULL)
		EvaluatedRootPoly[Process] = strtod(token, NULL);
	else
	{
		TraceInfo("%s*exit*Invalid Piece 2\n", __FUNCTION__);
		return 1;
	}

	TraceInfo("%s*exit\n", __FUNCTION__);
	return 0;
}

void printTables(int size, double X_1[], double Y_1[])
{
	#ifndef DEBUG
		return;
	#endif

	printf("\n");
	for (int i = 0; i < size; i++)
		printf("Xi[%f] ", X_1[i]);
	printf("\n");
	for (int i = 0; i < size; i++)
		printf("Yi[%f] ", Y_1[i]);
	printf("\n");
}

double CalculatePolynomial(double EvaluatedRootPoly[])
{
	int size = 0;
	int counter = 0;
	int status;

	for (int i = 0; i < numOfNodes; i++)
		if (outArray[i].code != 0 && (fabs(EvaluatedRootPoly[i]) >= 0.0001))
			size++;

	TraceDebug("%s*size:[%d]\n",__FUNCTION__, size);

	if (size <= 0)
	{
		TraceInfo("I can't do this man...\n");
		return 0;
	}

	double X_1[size], dummy_X[size];
	double Y_1[size], dummy_Y[size];
	double result;

	for (int i = 0; i < numOfNodes; i++)
	{
		if (outArray[i].code > 0  && i == 0)
		{
			X_1[i] = pow(RootOfUnity, numOfNodes);
			Y_1[i] = EvaluatedRootPoly[i];
			counter++;
		}
		else if (outArray[i].code > 0 )
		{
			X_1[counter] = pow(RootOfUnity, i);
			Y_1[counter] = EvaluatedRootPoly[i];
			counter++;
		}
	}

	parallel_array_merge_sort(0, size - 1, X_1, dummy_X, Y_1, dummy_Y);
	printTables(size,X_1,Y_1);

	// Disable gsl errors
	gsl_set_error_handler_off();

    gsl_interp_accel *acc = gsl_interp_accel_alloc();
    gsl_spline *spline = gsl_spline_alloc(gsl_interp_steffen, size);

    status = gsl_spline_init(spline, X_1, Y_1, size);

	if (status)
	{
		TraceInfo("Interpolation cannot be performed. ErrorCode[%d] Description:[%s]\n", status, gsl_strerror(status));
		return 0;
	}

	result = gsl_spline_eval(spline, 0, acc);
	gsl_spline_free(spline);
	gsl_interp_accel_free(acc);

	return result;
}
