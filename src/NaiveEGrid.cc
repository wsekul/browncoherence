#include "NaiveEGrid.h"

//history size, max salience level, smoothing (additive)
//generativity level
// = NOT -- assigns probability to production after eg [S S 2]
// = SLIGHTLY -- no end marker
// = MORE -- no probability for production after [S S 2]
// = REALLY -- salience represents previously-observed occurrence counts
NaiveEGrid::NaiveEGrid(int hist, int maxSal, 
					   double smooth, int howGenerative):
	EGridModel(hist, maxSal),
	_smooth(smooth),
	_normalized(false),
	_howGenerative(howGenerative)
{
}

NaiveEGrid::NaiveEGrid(istream& is):
	EGridModel(0, 0)
{
	read(is);
}

//two serialization methods: should be inverses of each other

//serializes this class
void NaiveEGrid::write(ostream& os)
{
	os<<"NAIVE\n";
	os<<_history<<"\t"<<_maxSalience<<"\t"<<_smooth<<"\t"<<
		_howGenerative<<"\n";
	os<<_normalized<<"\n";
	writeMap2(_probs, os);
}

//initializes this class -- called by the istream constructor
void NaiveEGrid::read(istream& is)
{
	string id;
	is>>id;
	assert(id == "NAIVE");

	is>>_history>>_maxSalience>>_smooth>>_howGenerative;
	is>>_normalized;
	readMap2(_probs, is);
}

void NaiveEGrid::print(ostream& os)
{
	int endSym = (_howGenerative == NOT_GEN) ? T_START : T_NONE;

	int ctxts = numContextTypes();
	for(int context = 0; context < ctxts; context++) 
	{
		if(!possible(numToHist(context)))
		{
			continue;
		}

		os<<histStr(numToHist(context))<<":\n";

		intToProb& counts = _probs[context];
		for(int sym = T_SUBJ; sym <= endSym; sym++)
		{
			os<<"\t"<<NP::roleToString(sym)<<":\t"<<counts[sym]<<"\n";
		}
		os<<"\n";
	}
}

//log probability of a permuted document
Prob NaiveEGrid::permProbability(Document& doc, ints& perm, bool train)
{
	_sentScores.clear();
	_sentScores.resize(perm.size());
	_globalScore = 0;

	if(train)
	{
		assert(!_normalized);
	}
	else
	{
		assert(_normalized);
	}

	Prob res = 0.0;

	for(symToIntToInt::iterator entity = _roles.begin();
		entity != _roles.end();
		entity++)
	{
		int occSoFar = 0;
		int totalOcc = _occurrences[entity->first];
		History hist;
		for(int i = 0; i<_history; i++)
		{
			hist.push_back(T_START);
		}

		int sent = 0;
		for(ints::iterator s = perm.begin();
			s != perm.end();
			s++,++sent)
		{
			int produced = _roles[entity->first][*s];

// 			cerr<<"egrid check "<<SYMTAB->toString(entity->first)<<" sent\n";
// 			cerr<<*doc[*s]<<"\n";
// 			cerr<<NP::roleToString(produced)<<"\n\n";

			if(_howGenerative == MORE_GEN &&
			   occSoFar == totalOcc &&
			   occSoFar < _maxSalience)
			{
				//we've produced our alloted number of these already
				assert(produced == T_NONE);
				break;
			}
			else
			{
				addSalience(occSoFar, totalOcc, hist);
				int histNum = histToNum(hist);

				if(train)
				{
					_probs[histNum][produced]++;
				}
				else
				{
					Prob term = log(_probs[histNum][produced]);
					res += term;
					_sentScores[sent] += term;					
				}
			}

			if(occSoFar < _maxSalience && produced != T_NONE)
			{
				occSoFar++;
			}
		
			//update the context
			hist.pop_back(); //boot out the salience
			hist.pop_front(); //and the oldest context item
			hist.push_back(produced); //stick in the newest context item
		}
// 		if(totalOcc != occSoFar)
// 		{
// 			cerr<<"Occurrence mismatch: "<<SYMTAB->toString(entity->first)<<
// 				" "<<totalOcc<<" cached occurrences, "<<
// 				occSoFar<<" counted occurrences.\n";
// 		}
// 		assert(totalOcc == occSoFar);

		//original system also produces an end marker... this is really
		//not generative, since it 'generates' the document length
		//independently for each entity
		//we will encode the end marker as T_START
		if(_howGenerative == NOT_GEN)
		{
			addSalience(occSoFar, totalOcc, hist);
			int histNum = histToNum(hist);

			int produced = T_START;

			if(train)
			{
				_probs[histNum][produced]++;
			}
			else
			{
				res += log(_probs[histNum][produced]);
			}
		}
	}

	if(train)
	{
		return log(0); //makes it harder to use train mode by mistake!
	}
	return res;
}

void NaiveEGrid::addSalience(int occSoFar, int totalOcc, History& hist)
{
	if(_howGenerative == REALLY_GEN)
	{
		//must be a number from 1..maxSalience
		if(occSoFar + 1 < _maxSalience)
		{
			hist.push_back(occSoFar+1);
		}
		else
		{
			hist.push_back(_maxSalience);
		}
	}
	else
	{
		hist.push_back(totalOcc);
	}
}

//estimate parameters
void NaiveEGrid::estimate()
{
	assert(!_normalized);
	_normalized = true;

	int endSym = (_howGenerative == NOT_GEN) ? T_START : T_NONE;

	int ctxts = numContextTypes();
	for(int context = 0; context < ctxts; context++) 
	{
		Prob norm = 0;
		intToProb& counts = _probs[context];

		for(int sym = T_SUBJ; sym <= endSym; sym++)
		{
			norm += _smooth + counts[sym];
		}

		for(int sym = T_SUBJ; sym <= endSym; sym++)
		{
			counts[sym] = (counts[sym] + _smooth) / norm;
		}
	}
}

