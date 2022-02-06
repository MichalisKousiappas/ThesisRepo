#ifndef INIT_H
#define INIT_H

#include "globals.h"

void init(void *context, 
		struct servers reqServer[],
		char serversIP[][256],
		double polynomials[][CONFIDENCE_PARAM][badPlayers],
		gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM],
		double RootPoly[],
		gsl_complex EvaluatedRootPoly[],
		double RootPolynomial[]);

void ValidateInput(int argc);

#endif