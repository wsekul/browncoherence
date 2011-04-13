class MultiwayDisentangler;

#ifndef MULTIWAY_DISENTANGLER_H
#define MULTIWAY_DISENTANGLER_H

#include "Disentangler.h"

class MultiwayDisentangler : public Disentangler
{
public:
	MultiwayDisentangler(CoherenceModel* model, Transcript* doc, bool verbose);

	virtual void solve();
	virtual void reset();
	virtual Prob rank();
	virtual Prob approxRank();

	virtual void tabooSolve();

	virtual Prob partialObjective(int sLast, int emptyThread);
	virtual Prob partialObjective(int sLast, intSet& live);
	virtual void greedySolve();
	virtual void greedyPartialSolve();

	virtual void recalc(int sent, int whereWent, int ti, int tTo);
	virtual Prob gainByMoving(int sent, int from, int target);
	virtual void evaluateMoves();
};

#endif
