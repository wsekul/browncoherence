class SpeakerModel;

#ifndef SPEAKER_MODEL_H
#define SPEAKER_MODEL_H

#include "CoherenceModel.h"

class SpeakerModel : public CoherenceModel
{
public:
	SpeakerModel(double alpha);
	SpeakerModel(istream& in);

	virtual ~SpeakerModel();

	virtual int history();

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

protected:
	double _alpha;
};

#endif
