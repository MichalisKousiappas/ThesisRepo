#ifndef GRADEDDECIDE_H
#define GRADEDDECIDE_H

#include "globals.h"

struct output SimpleGradedDecide(struct servers reqServer[],
						double polyEvals[][numOfNodes][CONFIDENCE_PARAM],
						double EvaluatedRootPoly[],
						double polynomials[numOfNodes][CONFIDENCE_PARAM][badPlayers],
						double RootPolynomial[badPlayers],
						double Secret_hj[][numOfNodes]);

#endif