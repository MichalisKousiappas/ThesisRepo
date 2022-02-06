#ifndef GRADEDDECIDE_H
#define GRADEDDECIDE_H

#include "globals.h"

void SimpleGradedDecide(struct servers reqServer[],
						gsl_complex polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						gsl_complex EvaluatedRootPoly[],
						double polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						double RootPolynomial[badPlayers]);

#endif