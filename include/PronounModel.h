class PronounModel;

#ifndef PRONOUN_MODEL_H
#define PRONOUN_MODEL_H

#include "CoherenceModel.h"
#include "PronounHandler.h"

//uses generative pronoun reference model to evaluate coherence
class PronounModel : public CoherenceModel
{
public:
	PronounModel(string& filename);
	PronounModel(istream& in);

	virtual ~PronounModel();

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
	PronounHandler _pron;
};

#endif
