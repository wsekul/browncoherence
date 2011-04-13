class JointModel;

#ifndef JOINT_MODEL_H
#define JOINT_MODEL_H

#include "CoherenceModel.h"

//joint distribution composed of multiple independent models
class JointModel : public CoherenceModel
{
public:
	JointModel();
	JointModel(istream& is);

	virtual ~JointModel();

	//two serialization methods: should be inverses of each other

	//serializes this class
	virtual void write(ostream& os);
	//initializes this class -- called by the istream constructor
	virtual void read(istream& is);

	//you are allowed to print debugging info using this method
	//it doesn't have to do anything
	virtual void print(ostream& os);

	virtual int history();

	//cache info about a document
	virtual void initCaches(Document& doc);
	virtual void clearCaches();

	//log probability of a permuted document
	//or does training
	virtual Prob permProbability(Document& doc, ints& perm, bool train);
	
	//estimate parameters
	virtual void estimate();

	virtual void addModel(CoherenceModel* model);
	virtual CoherenceModels& models();

protected:
	CoherenceModels _models;
};

#endif
