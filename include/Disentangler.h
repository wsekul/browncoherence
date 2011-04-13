class Disentangler;

#ifndef DISENTANGLER_H
#define DISENTANGLER_H

#include "common.h"
#include "CoherenceModel.h"
#include "Transcript.h"

typedef std::vector<Disentangler*> Disentanglers;

class Disentangler
{
public:
	Disentangler(CoherenceModel* model, Transcript* doc, bool verbose);

	virtual ~Disentangler();

	virtual Prob objective();

	virtual void solve();
	virtual void reset();

	virtual Prob rank();
	virtual Prob approxRank();

	virtual int bestMove(Prob& gain, intsSet& taboo, int& targetThread);
	virtual void tabooSolve();

	virtual void recalc(int sent, int whereWent, int ti, int tTo);
	virtual void evaluateMoves();
	virtual Prob gainByMoving(int sent, int from, int target);

	CoherenceModel* _model;
	Transcript* _transcript;
	bool _verbose;
	intProbsMap _threadBySent;
	intProbMap _threadGlobal;
	intIntProbMap _moves;
	intSet _fixed;
};

#endif
