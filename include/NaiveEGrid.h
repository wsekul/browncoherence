class NaiveEGrid;

#ifndef NAIVE_E_GRID_H
#define NAIVE_E_GRID_H

#include "EGridModel.h"

//my attempt to implement Lapata and Barzilay '04
class NaiveEGrid : public EGridModel
{
public:
	//history size, max salience level, smoothing (additive)
	//generativity level
	// = NOT -- assigns probability to production after eg [S S 2]
	// = SLIGHTLY -- no end marker
	// = MORE -- no probability for production after [S S 2]
	// = REALLY -- salience represents previously-observed occurrence counts
	NaiveEGrid(int hist, int maxSal, double smooth, int howGenerative);
	NaiveEGrid(istream& is);

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

protected:
	virtual void addSalience(int occSoFar, int totalOcc, History& hist);

	double _smooth;
	intToIntToProb _probs;
	bool _normalized;
	int _howGenerative;
};

#endif
