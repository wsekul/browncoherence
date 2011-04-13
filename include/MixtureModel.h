class MixtureModel;

#ifndef MIXTURE_MODEL_H
#define MIXTURE_MODEL_H

#include "JointModel.h"
#include "Transcript.h"
#include "MaxEntSelection.h"
#include "Disentangler.h"

//joint distribution composed of multiple independent models
class MixtureModel : public JointModel
{
public:
	MixtureModel();
	MixtureModel(istream& is);

	virtual void write(ostream& os);
	virtual void read(istream& is);

	virtual void initTrain(Transcript& trans);
	virtual Prob record(Transcript& trans, int sent, bool train,
						int bestState = 0, Prob score = 1.0);
	virtual Prob recordDual(Transcript& trans, int sent, bool train,
							int bestState = 0, Prob score = 1.0);
	virtual Prob recordInsertion(Document& doc, int sent);
	virtual Prob recordDiscrimination(Document& doc, ints& perm);

	//log probability of a permuted document
	//or does training
	virtual Prob permProbability(Document& doc, ints& perm, bool train);

	virtual void initCaches(Document& doc);
	virtual void clearCaches();

	//estimate parameters
	virtual void estimate();

	virtual void add(string flag, string file);

protected:
	MaxEntSelection _probs;
	Disentanglers _disentanglers;
	strings _args;
};

#endif
