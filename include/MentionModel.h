class MentionModel;

#ifndef MENTION_MODEL_H
#define MENTION_MODEL_H

#include "CoherenceModel.h"

enum {NO_MENTION, START_MENTION, CONTINUE_MENTION, UNKNOWN_MENTION};
enum {SPK_CONTINUING, SPK_BEGINNING};

class MentionModel : public CoherenceModel
{
public:
	MentionModel();
	MentionModel(istream& in);

	virtual ~MentionModel();

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
	intIntProbMap _counts;
	intIntMap _totals;
};

#endif
