class TimeModel;

#ifndef TIME_MODEL_H
#define TIME_MODEL_H

#include "CoherenceModel.h"

class TimeModel : public CoherenceModel
{
public:
	TimeModel(string& fname);
	TimeModel(istream& in);

	virtual ~TimeModel();

	//two serialization methods: should be inverses of each other

	//serializes this class
	virtual void write(ostream& os);
	//initializes this class -- called by the istream constructor
	virtual void read(istream& is);

	//cache info about a document
	virtual void initCaches(Document& doc);
	virtual void clearCaches();

	//log probability of a permuted document
	//or does training
	virtual Prob permProbability(Document& doc, ints& perm, bool train);
	
	//estimate parameters
	virtual void estimate();

	virtual Prob gapProb(int gap);

protected:
	intToProb _timeHist;
};

#endif
