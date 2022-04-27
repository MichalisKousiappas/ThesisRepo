#include "gradeddecide.h"
#include "polyfunc.h"
#include "gradecast.h"

//Local Function Declarations
void GetQueryBits(int node, double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double QueryBitsArray[], char result[]);
int ParseQueryBitsMessage(char *message, double array[][CONFIDENCE_PARAM]);
void PrepaireNewPolynomials(struct servers syncServer[],
						double QueryBitsArray[numOfNodes][CONFIDENCE_PARAM],
						double NewPolynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						double polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						double RootPolynomial[badPlayers]);
void BuildMessage(int node, double NewPolynomials[][CONFIDENCE_PARAM][badPlayers], char result[]);
int ParseMessage(int node, char *message, double NewPolynomials[][CONFIDENCE_PARAM][badPlayers]);
void PrintQueryBits(double QueryBitsArray[numOfNodes][CONFIDENCE_PARAM]);
int CheckForGoodPiece(double NewPolynomials[][CONFIDENCE_PARAM][badPlayers],
						double QueryBitsArray[][CONFIDENCE_PARAM],
						double polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						double EvaluatedRootPoly[],
						double RootPolynomial[badPlayers]);

/**
 * Start the Simple Graded - Decide phase
 */
struct output SimpleGradedDecide(struct servers reqServer[],
						double polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						double EvaluatedRootPoly[],
						double polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						double RootPolynomial[badPlayers],
						double Secret_hj[][numOfNodes])
{
	printf("-------------------SimpleGraded Decide----------------------------\n");
	//TraceInfo("%s*enter\n", __FUNCTION__);

	double QueryBitsArray[numOfNodes][CONFIDENCE_PARAM];
	double NewPolynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers];
	int GoodPieceMessages = 0, PassableMessages = 0;
	char GradedCastMessage[StringSecreteSize];
	char QBits[StringSecreteSize];
	char BuildedMessage[StringSecreteSize];
	char DecisionMessage[10] = {0};
	struct output DealersOutput;
	struct output accept;

	for (int i = 0; i < numOfNodes; i++)
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
			QueryBitsArray[i][j] = 0;

	memset(QBits, 0, sizeof(QBits));
	memset(BuildedMessage, 0, sizeof(BuildedMessage));
	memset(GradedCastMessage, 0, sizeof(GradedCastMessage));

	// all processes take turn and distribute their "secret"
	for (int distributor = 0; distributor < numOfNodes; distributor++)
	{
		if (TimedOut[distributor] == 1)
			continue;

		GetQueryBits(distributor, polyEvals, QueryBitsArray[proc_id], QBits);
		GradeCast(reqServer, distributor, QBits, outArray, GradedCastMessage);

		if (outArray[distributor].code > 0)
		{
			// Parse QueryBits, if message seems invalid then reject the sender
			if (ParseQueryBitsMessage(GradedCastMessage, QueryBitsArray))
			{
				outArray[distributor].code = 0;
				outArray[distributor].value = 0;
			}
		}
		#ifdef DEBUG
			printf("----------------------------------------\n");
		#endif
		memset(QBits, 0, sizeof(QBits));
		memset(GradedCastMessage, 0, sizeof(GradedCastMessage));
	}

	// Save the dealers output when he is pretending to be a normal processes
	DealersOutput = outArray[dealer];

	// Dealer will create the new polynomials while each node waits for the dealer to finish
	PrepaireNewPolynomials(reqServer, QueryBitsArray, NewPolynomials, polynomials, RootPolynomial);

	// Dealer now sends out the new polynomials for each node
	for (int procNum = 0; procNum < numOfNodes; procNum++)
	{
		BuildMessage(procNum, NewPolynomials, BuildedMessage);
		GradeCast(reqServer, dealer, BuildedMessage, outArray, GradedCastMessage);
				
		#ifdef DEBUG
			printf("----------------------------------------\n");
		#endif

		if (outArray[dealer].code > 0 && !IsDealer)
			ParseMessage(procNum, GradedCastMessage, NewPolynomials);
		
		memset(BuildedMessage, 0, sizeof(BuildedMessage));
		memset(GradedCastMessage, 0, sizeof(GradedCastMessage));
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

	// Set accept_ij output and secrete_hj
	accept = ValidateTally(PassableMessages);
	Secret_hj[proc_id][dealer] = EvaluatedRootPoly[proc_id];

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, PassableMessages);
	return accept;
}

/**
 * Random Query bits means check if bit 2 & 3 for example are set which is just
 * more complicated than just substracting a random number from all your numbers
 */
void GetQueryBits(int node, double polyEvals[][numOfNodes][CONFIDENCE_PARAM], double QueryBitsArray[], char result[])
{
	//TraceDebug("%s*enter\n", __FUNCTION__);
	int length = 0;
	int randomNum = rand() % (MAX_COEFICIENT/2);

	if (node != proc_id)
	{
		//TraceDebug("%s*exit*not my turn yet\n", __FUNCTION__);
		return;
	}

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

	//TraceDebug("%s*exit[%d]\n", __FUNCTION__, length);
}

/**
 * Parse the secret received from dealer and store it into array.
 * The secret are the evaulation of the polyonims
 */
int ParseQueryBitsMessage(char *message, double array[][CONFIDENCE_PARAM])
{
	//TraceDebug("ParseQueryBitsMessage [%s] size:[%ld]\n", message, strlen(message));

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
	//PrintQueryBits(QueryBitsArray);

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
		//printPolynomials(badPlayers, NewPolynomials, RootPolynomial);
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
void BuildMessage(int node, double NewPolynomials[][CONFIDENCE_PARAM][badPlayers], char result[])
{
	//TraceDebug("%s*enter\n", __FUNCTION__);
	int length = 0;

	if (!IsDealer)
	{
		//TraceDebug("%s*exit*Not my job\n", __FUNCTION__);
		return;
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

	//TraceDebug("%s*exit[%d]\n", __FUNCTION__, length);
}

/**
 * Parse the message received from dealer in step 2.
 */
int ParseMessage(int node, char *message, double NewPolynomials[][CONFIDENCE_PARAM][badPlayers])
{
	//TraceDebug("ParseMessage [%s] size:[%ld]\n", message, strlen(message));
	if (message[strlen(message) - 1] != '|')
	{
		TraceInfo("%s*exit*Invalid Message\n", __FUNCTION__);
		return 1;
	}

//	TraceDebug("%s*enter\n", __FUNCTION__);

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

	//TraceDebug("%s*exit\n", __FUNCTION__);
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

	//printPolynomials(badPlayers, NewPolynomials, RootPolynomial);

	if (proc_id == 0)
		power = numOfNodes;
	else
		power = proc_id;

	for (int i = 0; i < numOfNodes; i++)
	{
		if ((outArray[i].code == 0) && (i != proc_id))
			continue;

		// Enable in case you want excesive debugging.
		/*
		#ifdef DEBUG
			printf("i:[%d]\n",i);
		#endif
		*/
		for (int j = 0; j < CONFIDENCE_PARAM; j++)
		{
			counter1++;

			Pij = gsl_poly_eval(NewPolynomials[i][j], badPlayers, pow(RootOfUnity, power));
			TplusQmultiS = polyEvals[proc_id][i][j] + QueryBitsArray[i][j] * EvaluatedRootPoly[proc_id];
			//TraceDebug("i:[%d] j:[%d] Pij:[%f] TplusQmulitS:[%f] Qbit:[%f] RootPoly:[%f]\n",
			//			i, j, Pij, TplusQmultiS, QueryBitsArray[i][j], EvaluatedRootPoly[proc_id]);

			if ( fabs(Pij - TplusQmultiS) <= PRECISSION)
			{
				counter2++;
			}
			else
			{
				TraceDebug("Error here\n");
			}
		}
		/*
		#ifdef DEBUG
			printf("\n");
		#endif
		*/
	}
	res = ((counter1 == counter2) && (counter1 != 0));

	if (!res)
		EvaluatedRootPoly[proc_id] = 0;

	TraceInfo("%s*exit[%d][%d]\n", __FUNCTION__, res, dealer);
	return res;
}
