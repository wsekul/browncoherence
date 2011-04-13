class NaiveMaxEntEGridT;

#ifndef NAIVE_MAX_ENT_E_GRID_T_H
#define NAIVE_MAX_ENT_E_GRID_T_H

#include "NaiveMaxEntEGrid.h"
#include "Vocab.h"
#include "FeatureSet.h"
#include "LDA.h"

typedef std::tr1::unordered_map<stIndex, Probs> symProbsMap;

class NaiveMaxEntEGridT : public NaiveMaxEntEGrid
{
public:
	NaiveMaxEntEGridT(int hist, int maxSal, int howGenerative,
		string ldaPath);
	NaiveMaxEntEGridT(istream& is);

	virtual void write(ostream& os);
	virtual void read(istream& is);

	virtual void print(ostream& os);

	virtual void readExternalData();

	virtual void initCaches(Document& doc);
	virtual void clearCaches();

	virtual Prob permProbability(Document& doc, ints& perm, bool train);

	virtual void estimate();

	virtual void finishFeatures();
	virtual Prob readFeatureDump(string dump);

	string _ldaFile;
	LDA* _lda;
	IntVocab _vocab;

	ProbMat3 _cache;
	symProbsMap _nounVectors;
};

#endif
