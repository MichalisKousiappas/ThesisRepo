#ifndef VOTE_H
#define VOTE_H

#include "globals.h"

void Vote(struct servers reqServer[],
			struct output DecideOutput[][numOfNodes],
			struct output candidate[]);

#endif