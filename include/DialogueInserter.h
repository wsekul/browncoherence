class DialogueInserter;

#ifndef DIALOGUE_INSERTER_H
#define DIALOGUE_INSERTER_H

#include "Inserter.h"

class DialogueInserter : public Inserter
{
public:
	DialogueInserter(CoherenceModel* model, Document& doc, bool verbose);

	//returns preferred insertion pos for sentence k
	virtual int insert(int k);

	virtual double score(int pos, int k);

	void makePerm(int move, int pos, Document* doc, ints& perm);
};

#endif
