class PronounHandler;

#ifndef PRONOUN_HANDLER_H
#define PRONOUN_HANDLER_H

#include "common.h"
#include "Document.h"
#include "ge.h"

typedef std::vector<Sent*> Sents;

//provides some functions useful for using pronoun coreference
//in various models, without copying lots of code
class PronounHandler
{
public:
	PronounHandler();
	PronounHandler(const string& filename, Prob threshold);
	PronounHandler(istream& in);

	virtual ~PronounHandler();

	virtual Prob threshold();

	//two serialization methods: should be inverses of each other

	//serializes this class
	virtual void write(ostream& os);
	//initializes this class -- called by the istream constructor
	virtual void read(istream& is);

	//cache info about a document
	virtual void initCaches(Document& doc);
	virtual void clearCaches();

	//log probability of a permuted document
	virtual Prob permProbability(Document& doc, ints& perm);
	
	//update an entity grid with respect to a permuted document
	virtual Prob updateGrid(Document& doc, ints& perm, symToIntToInt& roles);

protected:
	virtual void readProbs();

	Prob _threshold;
	Sents _copy;
	string _probsFile;
	Ge _ge;
};

#endif
