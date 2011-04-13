#include "NewEntityModel.h"
#include "treeInfo.h"

NewEntityModel::NewEntityModel(int maxSalience, int maxBegin, bool byRef,
							   bool newHead):
	_maxSalience(maxSalience),
	_maxBegin(maxBegin),
	_byRef(byRef),
	_newHead(newHead)
//	_probs( (newHead ? 4 : 3), 0.0, RNG)
//	_probs( (newHead ? 3 : 2), 0.0, RNG)
{
	assert(_byRef || !_newHead);
}

NewEntityModel::NewEntityModel(istream& in)
//	_probs(0, 0.0, RNG)
{
	read(in);
}

NewEntityModel::~NewEntityModel()
{
}

//two serialization methods: should be inverses of each other

//serializes this class
void NewEntityModel::write(ostream& os)
{
	os<<"NEW_ENTITY\n";
	os<<_maxSalience<<"\t"<<_maxBegin<<"\t"<<_byRef<<"\t"<<_newHead<<"\n";
//	_probs.write(os);
}

//initializes this class -- called by the istream constructor
void NewEntityModel::read(istream& is)
{
	string key;
	is>>key;
	assert(key == "NEW_ENTITY");
	is>>_maxSalience>>_maxBegin>>_byRef>>_newHead;
//	_probs.read(is);
}

void NewEntityModel::setByRef(bool byRef)
{
	_byRef = byRef;
	assert(_byRef || !_newHead);
}

//you are allowed to print debugging info using this method
//it doesn't have to do anything
void NewEntityModel::print(ostream& os)
{
}

//cache info about a document
void NewEntityModel::initCaches(Document& doc)
{
}

void NewEntityModel::clearCaches()
{
}

int NewEntityModel::history()
{
	return GLOBAL_HIST;
}

//log probability of a permuted document
//or does training
Prob NewEntityModel::permProbability(Document& doc, ints& perm, bool train)
{
	_sentScores.clear();
	_sentScores.resize(perm.size());
	_globalScore = 0;

	Prob res = 0.0;

	symSet seenBefore;
	symSet seenHeadBefore;

	int len = perm.size();
	for(int ctr = 0; ctr < len; ctr++)
	{
		Sent* s = doc[perm[ctr]];

		for(NPs::iterator np = s->nps().begin();
			np != s->nps().end();
			np++)
		{
			if(!(*np)->discNewMarkable())
			{
				continue;
			}

			if(!isNoun(Sent::head((*np)->node())->label))
			{
				continue;
			}

			int occurrences;
			stIndex key;
			if(_byRef)
			{
				key = (unsigned int)(*np)->ref();
				occurrences = doc.byReferent().find(key)->second.size();
			}
			else
			{
				key = (*np)->headSym();
				occurrences = doc.byHead().find(key)->second.size();
			}

			symSet::iterator entry = seenBefore.find(key);
			seenBefore.insert(key);
			
			int label;
			if(_byRef && key == 0) 
			{
				//the 0 'referent' isn't real, assume no coreferents
				label = DISC_SINGLE;
			}
			else if(entry == seenBefore.end())
			{
				if(occurrences > 1)
				{
					label = DISC_INIT;
				}
				else
				{
					//singletons annotated with a referent:
					//this often happens when the coreferent NP
					//isn't markable
					//eg: MUC's annotation suggests the markings
					// (NP (NP-i a system) (VP called (NP-i VORTAC)))
					// while we use
					// (NP-i a system called VORTAC)
//					label = DISC_INIT;
					label = DISC_SINGLE;
				}
			}
			else
			{
				label = DISC_OLD;
			}

			bool isNewHead = contains(seenHeadBefore, (*np)->headSym());
			seenHeadBefore.insert((*np)->headSym());

// 			cerr<<"NP "<<(*np)->head()<<" label "<<
// 				label<<"\n";

			Prob term = npProb(*np, label, isNewHead, occurrences, ctr, train);
			res += term;
			_globalScore += term;
		}
	}

	if(train)
	{
		return log(0.0);
	}
	return res;
}

Prob NewEntityModel::npProb(NP* np, int label, bool isNewHead, 
							int occurrences, int sent, bool train)
{
	string strLabel = NP::discToString(label);

	StringList context;
//  	context.push_back(intToString(
//  						  Sent::proper(
//  							  Sent::head(np->node())->label)));
//  	context.push_back(intToString( (sent > _maxBegin) ? _maxBegin : sent));
//  	context.push_back(intToString( (occurrences > _maxSalience) ? 
// 								   _maxSalience : occurrences));
	context.push_back(SYMTAB->toString(np->node()->parent->label));

	if(_newHead)
	{
		string newHead = isNewHead ? "OLD_HEAD" : "NEW_HEAD";
		context.push_back(newHead);
	}

	if(train)
	{
//		_probs.addCount(context, strLabel);
		return log(0.0);
	}
	else
	{
		if(label == DISC_SINGLE)
		{
			return log(1.0);
		}
//		return log(_probs.prob(context, strLabel));
		return 0;
	}
}
	
//estimate parameters
void NewEntityModel::estimate()
{
//	_probs.em(.001);
}
