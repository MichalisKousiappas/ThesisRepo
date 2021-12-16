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
