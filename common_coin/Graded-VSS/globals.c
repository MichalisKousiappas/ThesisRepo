#include "globals.h"

void GetTime(char res[])
{
	time_t t;
	struct timeval timeVar;

	gettimeofday(&timeVar, NULL);
	time(&t);
	strftime(res, 21, "%d/%m/%Y %T", localtime(&t));
	sprintf(res+19, ".%ld", timeVar.tv_usec);
	res[23] = '\0'; //force null otherwise it will print more than 3 digits
}

/**
 *	Function that waits for a message to arrive thus avoiding sleep command.
 *	Must be paired with Distribue( server, "OK")
 */
void WaitForDealerSignal(struct servers syncServer[])
{
	char temp[4] = {0};

	TraceDebug("%s*enter\n", __FUNCTION__);
	zmq_recv(syncServer[proc_id].value, temp, 3, 0);
	TraceDebug("%s*exit[%s]\n", __FUNCTION__, temp);
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

/**
 * Simulate random death of nodes
*/
void RandomDeath()
{
	if (DEAD_NODES && (proc_id != 0) && (proc_id != dealer) && (proc_id%3 == 0))
	{
		if (((rand() + proc_id) % 10) == 0)
		{
			printf("Im die, thankyu fo eva\n");
			exit(-1);
		}
	}
}

/**
  Send the same message to all other nodes -- Doesn't wait for an answer
 */
void Distribute(struct servers reqServer[], const char *commonString)
{
	char sendBuffer[StringSecreteSize];

	memset(sendBuffer, 0, sizeof(sendBuffer));

	TraceDebug("%s*enter\n", __FUNCTION__);

	sprintf(sendBuffer, "%s", commonString);

	if (memcmp(commonString, "OK", 2) && commonString[0])
		messages++;

	//Distribute your message to all other nodes
	for (int i = 0; i < numOfNodes; i++)
	{
		if ((i == proc_id) || TimedOut[i] == 1) continue;

		//TraceDebug("Sending data as client[%d] to [%d]: [%s]\n", proc_id, i, sendBuffer);
		zmq_send(reqServer[i].value, sendBuffer, StringSecreteSize, 1);

		if (memcmp(commonString, "OK", 2) && commonString[0])
			messages++;
	}
	TraceDebug("%s*exit\n", __FUNCTION__);
}

/**
 *	Function to sort two parallel arrays based on the first array
 */
void parallel_array_merge_sort(int i, int j, double a[], double aux[], double b[], double bux[])
{
	if (j <= i)
		return;     // the subsection is empty or a single element

	int mid = (i + j) / 2;

	// left sub-array is a[i .. mid]
	// right sub-array is a[mid + 1 .. j]

	parallel_array_merge_sort(i, mid, a, aux, b, bux);     // sort the left sub-array recursively
	parallel_array_merge_sort(mid + 1, j, a, aux,b ,bux);     // sort the right sub-array recursively

	int pointer_left = i;       		// pointer_left points to the beginning of the left sub-array
	int pointer_right = mid + 1;        // pointer_right points to the beginning of the right sub-array
	int k;								// k is the loop counter

	// we loop from i to j to fill each element of the final merged array
	for (k = i; k <= j; k++) {
		if (pointer_left == mid + 1) {							// left pointer has reached the limit
			aux[k] = a[pointer_right];
			bux[k] = b[pointer_right];
			pointer_right++;
		} else if (pointer_right == j + 1) {					// right pointer has reached the limit
			aux[k] = a[pointer_left];
			bux[k] = b[pointer_left];
			pointer_left++;
		} else if (a[pointer_left] < a[pointer_right]) {		// pointer left points to smaller element
			aux[k] = a[pointer_left];
			bux[k] = b[pointer_left];
			pointer_left++;
		} else {												// pointer right points to smaller element
			aux[k] = a[pointer_right];
			bux[k] = b[pointer_right];
			pointer_right++;
		}
	}

	for (k = i; k <= j; k++) {	// copy the elements from aux[] to a[]
		a[k] = aux[k];
		b[k] = bux[k];
	}
}

void TimeoutDetected(const char *Who, int node)
{
	if (TimedOut[node] == 1)
		return;

	TraceInfo("%s*Timedout on [%d]\n", Who, node);
	TimedOut[node] = 1;
}