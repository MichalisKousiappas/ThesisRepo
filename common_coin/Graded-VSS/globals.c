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
	char temp[3] = {0};

	TraceDebug("%s*enter\n", __FUNCTION__);
	zmq_recv(syncServer[proc_id].value, temp, 3, 0);
	TraceDebug("%s*exit[%s]\n", __FUNCTION__, temp);
}
