#include "globals.h"

char *GetTime()
{
	time_t t;
	struct timeval timeVar;
	char *buf = (char *) malloc(25);
	memset(buf, 0, 40);

	gettimeofday(&timeVar, NULL);
	time(&t);
	strftime(buf, 21, "%d/%m/%Y %T", localtime(&t));
	sprintf(buf+19, ".%ld", timeVar.tv_usec);
	buf[23] = '\0'; //force null otherwise it will print more than 3 digits
	return buf;
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
