class NaiveMaxEntEGridF;

#ifndef NAIVE_MAX_ENT_E_GRID_F_H
#define NAIVE_MAX_ENT_E_GRID_F_H

#include "NaiveMaxEntEGrid.h"
#include "FeatureSet.h"

class NaiveMaxEntEGridF : public NaiveMaxEntEGrid
{
public:
	NaiveMaxEntEGridF(int hist, int maxSal, int howGenerative);
	NaiveMaxEntEGridF(istream& is);

	//two serialization methods: should be inverses of each other
	//serializes this class
	virtual void write(ostream& os);
	//initializes this class -- called by the istream constructor
	virtual void read(istream& is);

	virtual void readExternalData();

	virtual void print(ostream& os);

	//log probability of a permuted document
	virtual Prob permProbability(Document& doc, ints& perm, bool train);

	virtual void estimate();

	virtual void finishFeatures();
	virtual Prob readFeatureDump(string dump);

//protected:
	int F_PRIOR;
	int F_HIST;
	int F_UNLINK;
	int F_LINK;
	int F_PROPER;
	int F_NE;
	int F_MOD;
	int F_PLUR;
	int F_HASPRO;
	int F_NEVERPRO;
	int F_RELCLAUSE;
	int F_HEAD;

	strSet _unlinkable;
	strSet _linkable;

	strSet _unPro;
	strSet _hasPro;

	IntVocab _vocab;
};

#endif
