#ifndef INIT_H
#define INIT_H

#include "globals.h"

void init(char serversIP[][256]);
void PrepareConnections(void *context, struct servers reqServer[], char serversIP[][256]);
void ValidateInput(int argc);

#endif