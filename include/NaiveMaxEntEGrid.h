class NaiveMaxEntEGrid;

#ifndef NAIVE_MAX_ENT_E_GRID_H
#define NAIVE_MAX_ENT_E_GRID_H

#include "EGridModel.h"
#include "Vocab.h"
#include "FeatureSet.h"
#include "MaxEntSelection.h"

typedef std::map<stIndex, intToNP> symToIntToNP;

class NaiveMaxEntEGrid : public EGridModel
{
public:
	//history size, max salience level, smoothing (additive)
	//generativity level
	// = NOT -- assigns probability to production after eg [S S 2]
	// = SLIGHTLY -- no end marker
	// = MORE -- no probability for production after [S S 2]
	// = REALLY -- salience represents previously-observed occurrence counts
	NaiveMaxEntEGrid(int hist, int maxSal, int howGenerative);
	NaiveMaxEntEGrid(istream& is);

	//two serialization methods: should be inverses of each other

	//serializes this class
	virtual void write(ostream& os);
	//initializes this class -- called by the istream constructor
	virtual void read(istream& is);

	//prints marginal probs
	virtual void print(ostream& os);

	//log probability of a permuted document
	virtual Prob permProbability(Document& doc, ints& perm, bool train);
	
	//estimate parameters
	virtual void estimate();
	virtual void writeFeatures(bool on);
	virtual void finishFeatures();

	virtual Prob readFeatureDump(string name);

//protected:
	virtual void addSalience(int occSoFar, int totalOcc, History& hist);

	FeatureSet _fset;

	int _howGenerative;
	MaxEntSelection _probs;
};

#endif
