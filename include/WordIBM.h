class WordIBM;

#ifndef WORD_IBM_H
#define WORD_IBM_H

#include "IBM.h"

typedef std::map<int, NP*> intToNP;

class WordIBM : public IBM
{
public:
	WordIBM(int prevSents, double smooth, int nulls);
	WordIBM(istream& in);
	WordIBM(int prevSents, IBM& other);

	virtual Prob permProbability(Document& doc, ints& perm, bool train);

//protected:
	//gets available alignments in prev sentences
	virtual void getContextWords(Sent* sent, NounContext& res, bool add);
	//gets words that need to be aligned in the next sentence
	virtual void getProduced(Sent* sent, 
							 NounContexts& produced, bool train);

	virtual void allNs(Tree* node, NounContext& res, bool add);

	intToNP _mostRecent;
};

#endif
