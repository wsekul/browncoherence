#include "PronounHandler.h"
#include "stripTree.h"

PronounHandler::PronounHandler()
{
}

PronounHandler::PronounHandler(const string& filename, Prob threshold):
	_threshold(threshold),
	_probsFile(filename)
{
	readProbs();
}

PronounHandler::PronounHandler(istream& in)
{
	read(in);
}

PronounHandler::~PronounHandler()
{
}

//two serialization methods: should be inverses of each other

//serializes this class
void PronounHandler::write(ostream& os)
{
	os<<"PRONOUN_HANDLER\t"<<_threshold<<"\t"<<_probsFile<<"\n";
}

//initializes this class -- called by the istream constructor
void PronounHandler::read(istream& is)
{
	string key;
	is>>key;
	assert(key == "PRONOUN_HANDLER");
	is>>_threshold>>_probsFile;

	readProbs();
}

void PronounHandler::readProbs()
{
	ifstream in(_probsFile.c_str());
	if(!in.is_open())
	{
		cerr<<"Can't open "<<_probsFile<<"\n";
		abort();
	}

	_ge.readProbs(in, SYMTAB);
}

Prob PronounHandler::threshold()
{
	return _threshold;
}

//cache info about a document
void PronounHandler::initCaches(Document& doc)
{
	for(Document::iterator i = doc.begin(); i != doc.end(); i++)
	{
		Sent* copySent = new Sent(**i);
//		cerr<<*copySent<<"\n";
//		stripTree(copySent->tree());
//		cerr<<*copySent->tree()<<"\n";

		_copy.push_back(copySent);
	}
}

void PronounHandler::clearCaches()
{
	for(Document::iterator i = _copy.begin(); i != _copy.end(); i++)
	{
		delete *i;
	}

	_copy.clear();
}

//log probability of a permuted document
//or does training
Prob PronounHandler::permProbability(Document& doc, ints& perm)
{
	Trees permTrees;
	for(ints::iterator i = perm.begin();
		i != perm.end();
		i++)
	{
		permTrees.push_back(_copy[*i]->tree());
	}

	Prob res = _ge.procStory(permTrees);

	return res;
}

Prob PronounHandler::updateGrid(Document& doc, ints& perm, 
								symToIntToInt& roles)
{
	Prob res = permProbability(doc, perm);
//	cerr<<"Result: "<<res<<"\n";

	intToSym chains;
	for(Sents::iterator i = _copy.begin();
		i != _copy.end();
		i++)
	{
//		cerr<<*(*i)->tree()<<"\n";
		for(NPs::iterator np = (*i)->nps().begin();
			np != (*i)->nps().end();
			np++)
		{
			if((*np)->pronoun())
			{
				int ref = Sent::head((*np)->node())->refnum;
				intToSym::iterator entry = chains.find(ref);

				stIndex chain = 0;
				if(entry != chains.end())
				{
					chain = entry->second;
				}

				Prob pprob = Sent::head((*np)->node())->pprob;
//				cerr<<"Prob is "<<pprob<<"\n";

				if(pprob < _threshold)
				{
					chain = (*np)->normHeadSym();
				}

//				cerr<<"Coref, ref is "<<ref<<"\n";

				if(chain != 0)
				{
  					cerr<<(*np)->head()<<" coref in chain "<<
  						SYMTAB->toString(chain)<<"\n";

					//now DO grid things that don't exist
					//to create gridlines for unresolved prns
// 					if(roles.find(chain) == roles.end())
// 					{
// 						//if it's not griddable, don't grid it
// 						//cerr<<"And leaving now...\n";
// 						continue;
// 					}

					int inSent = (*i)->index();
					int prevRole = T_NONE;
					intToInt::iterator prev = roles[chain].find(inSent);
					if(prev != roles[chain].end())
					{
						prevRole = prev->second;
					}
					int currRole = (*np)->role();
					if(currRole < prevRole)
					{
//						cerr<<"Adding symbol to "<<inSent<<"\n";
						roles[chain][inSent] = currRole;
					}
// 					else
// 					{
// 						cerr<<"But current sym "<<
// 							NP::roleToString(prevRole)<<" has precedence\n";
// 					}

					int npRef = (*np)->node()->refnum;
					chains[npRef] = chain;

// 					cerr<<"Linking "<<npRef<<" to "<<
// 						SYMTAB->toString(chain)<<"\n";
				}
				else
				{
//					cerr<<(*np)->head()<<" coref out of chain.\n";
				}
			}
			else
			{
				int ref = (*np)->node()->refnum;
				intToSym::iterator entry = chains.find(ref);

				if(entry == chains.end())
				{
					chains[ref] = (*np)->normHeadSym();
//					cerr<<"New chain for "<<ref<<" head "<<
//						(*np)->normHead()<<"\n";
				}
			}				
		}
	}

	//set up empty gridlines for pronouns
	int docLen = doc.size();
	for(symToIntToInt::iterator entity = roles.begin();
		entity != roles.end();
		entity++)
	{
		intToInt& entityRoles = entity->second;
		for(int i=0; i<docLen; i++)
		{
			if(entityRoles.find(i) == entityRoles.end())
			{
				entityRoles[i] = T_NONE;
			}
		}
	}

	return res;
}
