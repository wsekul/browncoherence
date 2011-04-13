#include "EGridModel.h"

EGridModel::EGridModel(int hist, int sal):
	_history(hist),
	_maxSalience(sal)
{
}

EGridModel::~EGridModel()
{
}

int EGridModel::history()
{
	return _history;
}

//returns an upper bound on the number of histories that exist
int EGridModel::numContextTypes()
{
	//number of symbols that can occur in contexts ^ history size
	// * number of salience symbols
	return
		(int)pow((double)(T_START - T_SUBJ + 1), _history) * _maxSalience;
}

//converts a history to a number
int EGridModel::histToNum(History& hist)
{
        return histToNum(hist, _history);
}

//converts a history to a number
int EGridModel::histToNum(History& hist, int history)
{
        int res = 0;

        History::iterator i = hist.begin();
        int ctr = 0;

        //history
        for(;
                i != hist.end() && ctr < history;
                i++,ctr++)
        {
                res *= T_START - T_SUBJ + 1;
                res += *i;
        }

        if(i != hist.end())
        {
                res *= _maxSalience;
                res += (*i - 1);
        }

//      cerr<<histStr(res)<<" is "<<res<<"\n";

        return res;
}

//converts a number to a history
History EGridModel::numToHist(int num)
{
	History res;

	int occ = (num % _maxSalience) + 1;
	res.push_front(occ);
	num /= _maxSalience;

	for(int ctr = 0; ctr < _history; ctr++)
	{
		int sym = num % (T_START - T_SUBJ + 1);
		res.push_front(sym);
		num /= (T_START - T_SUBJ + 1);
	}

	return res;
}

//this function detects histories which can simply never occur, such as
// [S S salience = 1], or [S <s> ...]
bool EGridModel::possible(const History& hist)
{
	int symbols = 0;

	History::const_iterator i = hist.begin();
	int ctr = 0;

	//history
	bool seenNonStart = false;
	for(;
		i != hist.end() && ctr < _history;
		i++,ctr++)
	{
		if(*i != T_START)
		{
			seenNonStart = true;
		}
		else if(seenNonStart)
		{
			return false; //START following non-start
		}

		if(*i != T_NONE && *i != T_START)
		{
			symbols++;
		}
	}

	assert(i != hist.end());
	int sal = *i;

	//more symbols than allowed occurrences
	if(symbols > sal && sal < _maxSalience)
	{
		return false;
	}

	return true;
}

//for printing histories
string EGridModel::histStr(const History& hist)
{
	string res = "[";
	bool first = true;
	int ctr = 0;
	History::const_iterator i;
	for(i = hist.begin();
		i != hist.end() && ctr < _history;
		i++, ctr++)
	{
		if(first)
		{
			first = false;
		}
		else
		{
			res += " ";
		}

		res += NP::roleToString(*i);
	}

	if(i != hist.end()) //add a salience count
	{
		res += " ";
		std::ostringstream cvt;
		cvt<<*i;
		res += cvt.str();
		i++;
	}

	for(; i != hist.end(); i++) //all symbols from here
	{
		res += " " + NP::roleToString(*i);
	}

	res += "]";
	return res;
}

symToIntToInt& EGridModel::roles()
{
	return _roles;
}

//cache info about a document
void EGridModel::initCaches(Document& doc)
{
	cacheRoles(doc);
	cacheOccurrences(doc);
}

void EGridModel::cacheRoles(Document& doc)
{
	int docLen = doc.size();

	//this loop determines which entities are griddable
	symToNPs& entities = doc.byHead();
	for(symToNPs::iterator entity = entities.begin();
		entity != entities.end();
		entity++)
	{
		string entStr = SYMTAB->toString(entity->first);
		if(entStr == "_NUM" || isNum(entStr))
		{
			continue;
		}

		if(entity->second.empty())
		{
			continue;
		}

		NP* chainMember = entity->second.front();
	
		if(chainMember->pronoun())
		{
		// debug variant: excludes only possessives
//		if(Sent::head(chainMember->node())->label == prpdLabel)
//		{
			//don't grid pronouns
			continue;
		}

		for(int i=0; i<docLen; i++)
		{
			_roles[entity->first][i] = T_NONE;
		}
		_reps[entity->first] = entity->second;
	}

// 	for(symToIntToInt::iterator ent = _roles.begin();
// 		ent != _roles.end();
// 		ent++)
// 	{
// 		intToInt& entRoles = ent->second;
// 		NPs& entNPs = entities[ent->first];

// 		_reps[ent->first] = entNPs;

// 		for(NPs::iterator np = entNPs.begin();
// 			np != entNPs.end();
// 			np++)
// 		{
// 			int inSent = (*np)->parent()->index();
// 			int prevRole = entRoles[inSent];
// 			int currRole = (*np)->role();
// 			if(currRole < prevRole)
// 			{
// 				entRoles[inSent] = currRole;
// 			}
// 		}
// 	}

	int n = doc.size();
	for(int si = 0; si < n; ++si)
	{
		Sent* sent = doc[si];
		foreach(NPs, np, sent->nps())
		{
			symToIntToInt::iterator entry = 
				_roles.find((*np)->normHeadSym());
			if(entry != _roles.end())
			{
				int prevRole = entry->second[si];
				int currRole = (*np)->role();
				if(currRole < prevRole)
				{
					entry->second[si] = currRole;
				}
			}
		}
	}
}

void EGridModel::cacheOccurrences(Document& doc)
{
	//used by not-fully generative models
	//number of sentences in which this head occurs
	for(symToIntToInt::iterator ent = _roles.begin();
		ent != _roles.end();
		ent++)
	{
		int occ = 0;
		for(intToInt::iterator i = ent->second.begin();
			i != ent->second.end();
			i++)
		{
			if(i->second != T_NONE)
			{
				occ++;
			}
		}

		_occurrences[ent->first] = 
			((occ < _maxSalience) ? occ : _maxSalience);

// 		cerr<<SYMTAB->toString(ent->first)<<" "<<
// 			_occurrences[ent->first]<<"\n";
	}

// 	symToNPs& entities = doc.byHead();
// 	for(symToNPs::iterator entity = entities.begin();
// 		entity != entities.end();
// 		entity++)
// 	{
// 		for(int i=0; i<docLen; i++)
// 		{
// 			if(_roles[entity->first][i] != T_NONE && 
// 			   _occurrences[entity->first] < _maxSalience)
// 			{
// 				_occurrences[entity->first]++;
// 			}
// 		}
// 	}
}

void EGridModel::clearCaches()
{
	_roles.clear();
	_occurrences.clear();
	_reps.clear();
}
