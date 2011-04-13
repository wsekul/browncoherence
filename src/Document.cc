#include "Document.h"

Document::Document(string& name)
{
	_name = name;
	ifstream in(name.c_str());
	if(!in.is_open())
	{
		cerr<<"Can't open "<<name<<"\n";
		abort();
	}
	read(in);
}

Document::Document(char* name)
{
	_name = string(name);
	ifstream in(name);
	if(!in.is_open())
	{
		cerr<<"Can't open "<<name<<"\n";
		abort();
	}
	read(in);
}

Document::Document(istream& is, string& name)
{
	_name = name;
	read(is);
}

Document::~Document()
{
	for(iterator i = begin();
		i != end();
		i++)
	{
		delete *i;
	}
}

string Document::name()
{
	return _name;
}

int Document::segments()
{
	return _segments.size() - 1;
}

ints& Document::segmentStarts()
{
	return _segments;
}

intToNPSet& Document::byReferent()
{
	return _byReferent;
}

intToNP& Document::byID()
{
	return _byID;
}

symToNPs& Document::byHead()
{
	return _byHead;
}

NPSet& Document::markables()
{
	return _markables;
}

NPSet& Document::nps()
{
	return _nps;
}

void Document::print(ostream& os)
{
	int seg = 0;
 	for(iterator j = begin();
 		j != end();
 		j++)
 	{
		if((*j)->index() == _segments[seg])
		{
			os<<"\t";
			seg++;
		}
 		os<<**j<<"\n";
 	}
}

void Document::read(istream& is)
{
	int ctr = 0;
	int speaker = -1;
	int dial = -1;
	int time = -1;
	intSet nextSpeakers;
	bool nextIsSegHeader = true;

	TreeToNP npsByTree;
	NPSet badAppositive;

	while(!is.eof() && !Tree::newstory)
	{
		Sent* s = new Sent(*this, ctr, speaker, dial, time, nextSpeakers);
		is>>*s;

		if(!is.eof() && !Tree::newstory)
		{
			if(s->tree() == NULL)
			{
				delete s;
				continue;
			}

			if(Sent::headWord(s->tree()) == "end-of-article")
			{
				delete s;
				break;
			}

			if(s->tree()->subtrees->label == codeLabel)
			{
				Tree* symSym = 
					Sent::hasChild(s->tree()->subtrees,
								   SYMTAB->toIndex("SYM"));
				int newSpeaker = speaker;
				if(symSym)
				{
					string speakerName = Sent::headWord(symSym);
					string sbit = speakerName.substr(
						string("speaker").length());
					newSpeaker = 0;
					for(int ii = 0; ii < sbit.size(); ii++)
					{
						newSpeaker *= 26;
						char ch = sbit[ii];
						newSpeaker += ch - 'a';
					}

//					cerr<<speakerName<<" "<<sbit<<" "<<newSpeaker<<"\n";

// 					newSpeaker = 
// 						speakerName[string("speaker").length()] - 'a';
				}

				if(speaker != newSpeaker)
				{
					speaker = newSpeaker;
					nextIsSegHeader = true;
				}

				Tree* dialSym = 
					Sent::hasChild(s->tree()->subtrees,
								   SYMTAB->toIndex("DIAL"));
				if(dialSym)
				{
					string dialName = Sent::headWord(dialSym);
					dial = atoi(dialName.c_str());
				}

				Tree* timeSym = 
					Sent::hasChild(s->tree()->subtrees,
								   SYMTAB->toIndex("TIME"));
				if(timeSym)
				{
					string timeName = Sent::headWord(timeSym);
					time = atoi(timeName.c_str());
				}

				nextSpeakers.clear();
				Tree* mentSym =
					Sent::hasChild(s->tree()->subtrees,
								   SYMTAB->toIndex("MENT"));
				if(mentSym)
				{
//					cerr<<"Block detected "<<*mentSym<<"\n";
					for(Tree* st = mentSym->subtrees;
						st != NULL;
						st = st->sibling)
					{
						string sbit = Sent::headWord(st);
						newSpeaker = 0;
						for(int ii = 0; ii < sbit.size(); ii++)
						{
							newSpeaker *= 26;
							char ch = sbit[ii];
							newSpeaker += ch - 'a';
						}
//						cerr<<"Name loaded "<<newSpeaker<<"\n";
						nextSpeakers.insert(newSpeaker);
					}
				}

				delete s;
				continue;
			}

			if(nextIsSegHeader)
			{
				_segments.push_back(ctr);
				nextIsSegHeader = false;
			}

			ctr++;

			push_back(s);

			NPs& nps = s->nps();
			for(NPs::iterator np = nps.begin();
				np != nps.end();
				np++)
			{
				//read the old refnum format (refnums on the tree nodes)
				//as well as the new separate-key format
				if((*np)->ref() != -1)
				{
					_markables.insert(*np);
					NPSet& refNPs = _byReferent[(*np)->ref()];

					if(refNPs.size() == 1)
					{
						(*refNPs.begin())->setStatus(DISC_INIT);
					}
					
					if(refNPs.size() >= 1)
					{
						(*np)->setStatus(DISC_OLD);
					}

					refNPs.insert(*np);
				}

				_nps.insert(*np);
				_byHead[(*np)->normHeadSym()].push_back(*np);
				npsByTree[Sent::head((*np)->node())] = *np;
				if(Sent::appositive((*np)->node()))
				{
					Tree* par = (*np)->node()->parent;
					assert(par != NULL);
					if(npsByTree.find(Sent::head(par)) != npsByTree.end())
					{
						NP* parNP = npsByTree[Sent::head(par)];
						if(!contains(badAppositive, parNP))
						{
							if(!parNP->setAppositive(*np))
							{
								badAppositive.insert(parNP);
							}
						}
					}
				}
			}
		}
		else
		{
			delete s;
		}
	}

	_segments.push_back(ctr);
	Tree::newstory = 0;
}

void Document::readMarkablesKey(istream& is, bool onlyParserMentions)
{
	NPs resolveNP; //parallel lists-- how hackish!
	ints resolvePrev;

	string word;
	while(is>>word)
	{
		if(word == ">>")
		{
			break;
		}

		int wordInd;
		int id;
		int prev;
		int group;
		is>>wordInd>>id>>prev>>group;

		if(wordInd == -1)
		{
			//a markable from the date section (gack!)...
			//this isn't even *in* the parsed data
			continue;
		}

//		cerr<<word<<"\t"<<wordInd<<"\t"<<id<<"\t"<<prev<<"\t"<<group<<"\n";

		NP* np = findNP(word, wordInd, onlyParserMentions);
		if(onlyParserMentions)
		{
			if(np == NULL)
			{
				continue;
			}
			else
			{
				assert(np != NULL);
			}
		}
		if(np->ref() == -1)
		{
			np->setRef(group);
		}
		else if(onlyParserMentions)
		{
			cerr<<"Skipping "<<*np<<" "<<id<<"\n";
			//two COREF tags on the same NP-- we'll ignore this one
			continue;
		}

		np->setMarkable(true);
		if(_byReferent[group].empty())
		{
			np->setStatus(DISC_INIT);
		}
		else
		{
			np->setStatus(DISC_OLD);
		}

 		_byID[id] = np;
 		_markables.insert(np);
 		_byReferent[group].insert(np);
		_nps.insert(np);

		if(prev != 0)
		{
			resolveNP.push_back(np);
			resolvePrev.push_back(prev);
		}
	}

	for(int i=0; i < resolveNP.size(); i++)
	{
		NP* np = resolveNP[i];
		int prev = resolvePrev[i];
		if(_byID.find(prev) != _byID.end())
		{
			np->setPrev(_byID[prev]);
		}
	}
}

NP* Document::findNP(string& word, int count, bool onlyParserMentions)
{
	stIndex label = SYMTAB->toIndex(word);

	int seen = 0;
	Tree* wordTree = NULL;
	Sent* wordSent = NULL;
	for(Document::iterator sent = begin();
		sent != end();
		sent++)
	{
		Trees leaves;
		Sent::getLeaves((*sent)->tree(), leaves);
		for(Trees::iterator tr = leaves.begin();
			tr != leaves.end();
			tr++)
		{
//			cerr<<**tr<<"\n";
//			if(endswith(Sent::word((*tr)->subtrees), word))
			if((*tr)->subtrees->label == label)
			{
				seen++;
				if(seen == count)
				{
					wordTree = (*tr);
					wordSent = *sent;
					break;
				}
			}
		}

		if(wordTree != NULL)
		{
			break;
		}
	}

	if(wordTree == NULL)
	{
		cerr<<"Couldn't find "<<word<<" "<<count<<"\n";
		cerr<<*this<<"\n";
	}
	assert(wordTree != NULL);
//	cerr<<"for "<<word<<" count "<<count<<" found "<<*wordTree<<"\n";
//	cerr<<*wordSent<<"\n";

	//some code to make sure NPs are inserted in top-down L-to-R order
	NPs::iterator firstOnRight = wordSent->nps().end();
	Trees leaves;
	Sent::getLeaves(wordSent->tree(), leaves);
	TreeToInt leavesOrdered;
	for(int ctr = 0; ctr < leaves.size(); ctr++)
	{
		leavesOrdered[leaves[ctr]] = ctr;
	}
	int wordIdx = leavesOrdered[wordTree];

	NP* wordNP = NULL;
	for(NPs::iterator sentNP = wordSent->nps().begin();
		sentNP != wordSent->nps().end();
		sentNP++)
	{
		if(Sent::head((*sentNP)->node()) == wordTree)
		{
			wordNP = *sentNP;
			break;
		}
		else if(firstOnRight == wordSent->nps().end())
		{
			if(!Sent::isAncestor(wordTree, (*sentNP)->node()))
			{
				//either left or right
				int headIdx = leavesOrdered[Sent::head((*sentNP)->node())];
				if(headIdx > wordIdx)
				{
					firstOnRight = sentNP;
				}
			}
		}				
	}

	if(!wordNP)
	{
		if(onlyParserMentions)
		{
			return NULL;
		}
		else
		{
			wordNP = new NP(wordSent, wordTree, wordSent->nps().size(),
				false);
			cerr<<"Creating np "<<Sent::plaintext(wordTree)<<"in\n";
			cerr<<Sent::plaintext(wordTree->parent)<<"\n\n";
			wordSent->_nps.insert(firstOnRight, wordNP);
		}
	}

	return wordNP;
}

int Document::entities()
{
	return _byReferent.size();
}

void Document::writeMarkablesKey(ostream& os)
{
	//not necessarily 1-to-1 mapping
	NPToInt npToID;
	for(intToNP::iterator np = _byID.begin();
		np != _byID.end();
		np++)
	{
		npToID[np->second] = np->first;
	}

	for(intToNP::iterator np = _byID.begin();
		np != _byID.end();
		np++)
	{
		assert(np->second);

		//XXX supposed to be invariant that this can't happen?
		if(!np->second->markable())
		{
			continue;
		}

		int prevID = -1;
		if(np->second->prev())
		{
			NPToInt::iterator entry = npToID.find(np->second->prev());
			if(entry != npToID.end())
			{
				prevID = entry->second;
			}
		}
		//word, (dummy) word ind, id, (real?) prev ind, ref
		os<<np->second->head()<<" 0 "<<np->first<<" "<<prevID<<" "<<
			np->second->ref();
		if(Sent::proper(Sent::headTagSym(np->second->node())))
		{
			os<<" P";
		}
		else
		{
			os<<" N";
		}
		os<<"\n";

		//sanity!
		if(np->second->ref() == -1)
		{
			cerr<<"WARNING: referent of -1 detected."<<
				" Remember that refnums are expected to be unique!\n";
		}
	}
}

void Document::makeAllMarkable()
{
	int maxIndex = 0;
	if(!_byID.empty())
	{
		maxIndex = _byID.rend()->first;
	}
	int nextInd = maxIndex + 100; //let's give a little bit of a gap

	for(Document::iterator sent = begin();
		sent != end();
		sent++)
	{
		for(NPs::iterator np = (*sent)->nps().begin();
			np != (*sent)->nps().end();
			np++)
		{
			if(!contains(_markables, *np))
			{
				(*np)->setMarkable(true);
				_markables.insert(*np);
				_byID[nextInd] = *np;
				(*np)->setRef(nextInd);
				_byReferent[nextInd].insert(*np);
				nextInd++;
			}
		}
	}
}

void Document::printWithRefs(ostream& os, Tree* tree, TreeToInt& treeToRef)
{
	TreeToInt::iterator entry = treeToRef.find(tree);
	if(entry != treeToRef.end())
	{
		os<<"<COREF id=\"0\" ref=\""<<entry->second<<"\"> ";
	}

	if(Sent::leaf(tree))
	{
		os<<Sent::word(tree)<<" ";
	}
	else
	{
		for(Tree* st = tree->subtrees;
			st != NULL;
			st = st->sibling)
		{
			printWithRefs(os, st, treeToRef);
		}
	}

	if(entry != treeToRef.end())
	{
		os<<"</COREF>";
	}
}

void Document::clearReferenceInfo()
{
	for(NPSet::iterator np = _markables.begin();
		np != _markables.end();
		np++)
	{
		(*np)->setRef(-1);
		(*np)->setStatus(DISC_SINGLE);
	}
	_byReferent.clear();
}

void Document::clearPredecessorLinks()
{
	for(NPSet::iterator np = _markables.begin();
		np != _markables.end();
		np++)
	{
		(*np)->setPrev(NULL);
	}
}

ostream& operator <<(ostream& os, Document& doc)
{
	doc.print(os);
	return os;
}

bool skipDocument(istream& is)
{
	if(!Tree::singleLineReader)
	{
		cerr<<"Skipping is currently only enabled in single-line mode.\n";
		abort();
	}

	string line;
	int ctr = 0;

	do
	{
		getline(is, line);
		ctr++;
	}while(!is.eof() && line != "" && line != "(S1 (S (NN end-of-article)))");

	if(ctr > 1)
	{
		return true;
	}
	return false;
}
