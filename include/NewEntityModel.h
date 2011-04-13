class NewEntityModel;

#ifndef NEW_ENTITY_MODEL_H
#define NEW_ENTITY_MODEL_H

#include "CoherenceModel.h"
//#include "SmoothedDist.h"

class NewEntityModel : public CoherenceModel
{
public:
	NewEntityModel(int maxSalience, int maxBegin, bool byRef, bool useNewHead);
	NewEntityModel(istream& in);

	virtual ~NewEntityModel();

	virtual int history();

	virtual void setByRef(bool byRef);

	//two serialization methods: should be inverses of each other

	//serializes this class
	virtual void write(ostream& os);
	//initializes this class -- called by the istream constructor
	virtual void read(istream& is);

	//you are allowed to print debugging info using this method
	//it doesn't have to do anything
	virtual void print(ostream& os);

	//cache info about a document
	virtual void initCaches(Document& doc);
	virtual void clearCaches();

	//log probability of a permuted document
	//or does training
	virtual Prob permProbability(Document& doc, ints& perm, bool train);
	
	//estimate parameters
	virtual void estimate();

protected:
	virtual Prob npProb(NP* np, int label, bool isNewHead,
						int occurrences, int sent, bool train);

	int _maxSalience;
	int _maxBegin;
	bool _byRef;
	bool _newHead;

//	SmoothedDist _probs;
};

#endif
