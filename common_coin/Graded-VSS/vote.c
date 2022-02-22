#include "vote.h"
#include "gradecast.h"
#include "math.h"

// Local Function Declaration
void GetAcceptList(int node, char result[]);
int ParseAcceptList(char *message, struct output DecideOutput);

/**
 * Vote
*/
void Vote(struct servers reqServer[],
			struct output DecideOutput,
			struct output candidate[])
{
	char GradedCastMessage[StringSecreteSize];
	char List[StringSecreteSize];

	memset(GradedCastMessage, 0, sizeof(GradedCastMessage));
	memset(List, 0, sizeof(List));

	printf("-------------------Vote----------------------------\n");
	// all processes take turn and distribute their "secret"
	for (int distributor = 0; distributor < numOfNodes; distributor++)
	{
		GetAcceptList(distributor, List);
		GradeCast(reqServer, distributor, List, candidate, GradedCastMessage);

		if (candidate[distributor].code > 0)
		{
			// Parse List, if message seems invalid then reject the sender
			if (ParseAcceptList(GradedCastMessage, DecideOutput))
			{
				candidate[distributor].code = 0;
				candidate[distributor].value = 0;
			}
		}
		printf("----------------------------------------\n");
		memset(GradedCastMessage, 0, sizeof(GradedCastMessage));
		memset(List, 0, sizeof(List));
	}
}

/**
 * Return the accept list in string
*/ 
void GetAcceptList(int node, char result[])
{
	TraceInfo("%s*enter\n", __FUNCTION__);
	int length = 0;

	if (node != proc_id)
	{
		TraceDebug("%s*exit*not my turn yet\n", __FUNCTION__);
		return;
	}

	length += snprintf(result+length , StringSecreteSize-length, "%d%s", proc_id, MESSAGE_DELIMITER);

	for(int i = 0; i < numOfNodes; i++)
	{
		length += snprintf(result+length , StringSecreteSize-length, "%d%s%d%s", outArray[i].code, MESSAGE_ACCEPT, outArray[i].value, MESSAGE_DELIMITER);
	}

	Traitor(result);

	//Finish the message with delimiter so parsing can be done correctly
	length += snprintf(result+length , StringSecreteSize-length, "%s", MESSAGE_DELIMITER);
	result[length-1] = '\0';

	TraceInfo("%s*exit[%d]\n", __FUNCTION__, length);
}

/**
 * Parse the accept list and check if candidate is valid
*/ 
int ParseAcceptList(char *message, struct output DecideOutput)
{
	TraceDebug("ParseAcceptList [%s] size:[%ld]\n", message, strlen(message));

	struct output ParsedMessage[numOfNodes];
	int counter = 0;

	if (message[strlen(message) - 1] != '|')
	{
		TraceInfo("%s*exit*Invalid accpet list\n", __FUNCTION__);
		return 1;
	}

	char* token = strtok(message, ALL_MESSAGE_DELIMITERS);
	//int Process_id = atoi(token); // not needed for now

	// Parse message
	for (int j = 0; j < numOfNodes; j++)
	{
		for(int d = 0; d < 2; d++)
		{
			token = strtok(0, ALL_MESSAGE_DELIMITERS);
			if (token != NULL && d == 0)
				ParsedMessage[j].code = atoi(token);
			else if (token != NULL && d == 1)
				ParsedMessage[j].value = atoi(token);
			else
			{
				TraceInfo("%s*exit*Invalid Accept list[%d]\n", __FUNCTION__, j);
				return 1;
			}
		}
	}

	#ifdef DEBUG
		for (int i = 0; i < numOfNodes; i++)
			printf("%s*i[%d] code[%d] value[%d]\n", __FUNCTION__, i, ParsedMessage[i].code, ParsedMessage[i].value);
	#endif

	// Count the number of nodes that were accepted
	for (int i = 0; i < numOfNodes; i++)
		if (ParsedMessage[i].code == 2)
			counter++;

	/*
		Check if the number of accepted nodes are more or equal to (n-t).
		Check if your code matches their code of you.
	*/
	if (!((counter >= (numOfNodes - badPlayers)) && (fabs(ParsedMessage[proc_id].code - DecideOutput.code) < 2)))
		return 1;

	TraceDebug("%s*counter[%d]\n", __FUNCTION__, counter);
	return 0;
}
