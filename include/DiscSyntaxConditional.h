class DiscSyntaxConditional;

#ifndef DISC_SYNTAX_CONDITIONAL_H
#define DISC_SYNTAX_CONDITIONAL_H

#include "NewEntityModel.h"
#include "MaxEntSelection.h"
#include "Vocab.h"

class DiscSyntaxConditional : public NewEntityModel
{
public:
	DiscSyntaxConditional(bool byRef, bool useNewHead);
	DiscSyntaxConditional(istream& in);
	virtual ~DiscSyntaxConditional();

	//two serialization methods: should be inverses of each other

	//serializes this class
	virtual void write(ostream& os);
	//initializes this class -- called by the istream constructor
	virtual void read(istream& is);

	//estimate parameters
	virtual void estimate();

protected:
	virtual Prob npProb(NP* np, int label, bool isNewHead,
						int occurrences, int sent, bool train);

	virtual void init();

	MaxEntSelection _probs;
	Vocab _feats;
};

#endif
