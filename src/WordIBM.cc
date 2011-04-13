#include "WordIBM.h"

WordIBM::WordIBM(int prevSents, double smooth, int nulls):
	IBM(smooth, nulls)
{
	_prevSents = prevSents;
}

WordIBM::WordIBM(int prevSents, IBM& other):
	IBM(other)
{
	_prevSents = prevSents;
}

WordIBM::WordIBM(istream& in):
	IBM(0, 0)
{
	read(in);
//	VERBOSE = 2;
}

void WordIBM::getContextWords(Sent* sent, NounContext& res, bool add)
{
	allNs(sent->tree(), res, add);
}

void WordIBM::getProduced(Sent* sent, NounContexts& res, bool add)
{
//	cerr<<*sent<<"\n";
	NounContext dummy;
	allNs(sent->tree(), dummy, add);

	foreach(NPs, np, sent->nps())
	{
		int vind = _vocab.get((*np)->headSym(), add);
		if(vind == -1)
		{
			continue;
		}

		int wInd = Sent::wordsBefore((*np)->node());
		res.push_back(NounContext(vind, (*np)->node()));
		NounContext& created = res.back();

// 		cerr<<"Making produced entry for "<<(*np)->head()<<"\n";
// 		cerr<<"its leftmost word is "<<wInd<<"\n";

		foreach(NounContext, possC, dummy)
		{
// 			cerr<<"possible to add "<<*possC->tree<<" with left "<<
// 				possC->wordInd<<"\n";

			if(possC->wordInd < wInd)
			{
//				cerr<<"add\n";
				created.push_back(*possC);
			}
		}

//XXX this block is cute but generatively illegitimate
// 		intToNP::iterator entry = _mostRecent.find(vind);
// 		if(entry != _mostRecent.end())
// 		{
// 			NP* mostRecent = entry->second;
// 			created.push_back(ContextWord(vind, mostRecent->node()));
// 			ContextWord& selfContext = created.back();
// 			selfContext.overrideSent = 
// 				sent->index() - mostRecent->parent()->index();
// 		}

		_mostRecent[vind] = *np;
	}
}

Prob WordIBM::permProbability(Document& doc, ints& perm, bool train)
{
	Prob res = IBM::permProbability(doc, perm, train);
	_mostRecent.clear();
	return res;
}

void WordIBM::allNs(Tree* node, NounContext& res, bool add)
{
	if(Sent::preterm(node))
	{
		char type = Sent::tag(node)[0];
		if(type == 'N' || type == 'V')
		{
			int key = _vocab.get(node->subtrees->label, add);
			if(key != -1)
			{
				res.push_back(ContextWord(key, node));
			}
		}
	}
	else
	{
		for(Tree* i = node->subtrees;
			i != NULL;
			i = i->sibling)
		{
			allNs(i, res, add);
		}
	}
}
