#include "NaiveMaxEntEGrid.h"

//history size, max salience level, smoothing (additive)
//generativity level
// = NOT -- assigns probability to production after eg [S S 2]
// = SLIGHTLY -- no end marker
// = MORE -- no probability for production after [S S 2]
// = REALLY -- salience represents previously-observed occurrence counts
NaiveMaxEntEGrid::NaiveMaxEntEGrid(int hist, int maxSal, 
								   int howGenerative):
	EGridModel(hist, maxSal),
	_howGenerative(howGenerative),
//	_probs(.5, .9999999, .0001)
	_probs(0)
{
	int nProduced = T_NONE - T_SUBJ + 1;
	_fset.addType(string("produced"), nProduced);
	for(int ii = 0; ii < hist; ++ii)
	{
		//one more for T_START
		_fset.addType(string("hist")+intToString(ii), nProduced + 1);
	}
	_fset.addType(string("salience"), maxSal + 1);
}

NaiveMaxEntEGrid::NaiveMaxEntEGrid(istream& is):
	EGridModel(0, 0),
//	_probs(false)
	_probs(0)
{
	read(is);
}

//two serialization methods: should be inverses of each other

//serializes this class
void NaiveMaxEntEGrid::write(ostream& os)
{
	os<<"NAIVEMAXENT\n";
	os<<_history<<"\t"<<_maxSalience<<"\t"<<
		_howGenerative<<"\n";
	_probs.write(os);
	_fset.write(os);
}

//initializes this class -- called by the istream constructor
void NaiveMaxEntEGrid::read(istream& is)
{
	string id;
	is>>id;
	assert(id == "NAIVEMAXENT");

	is>>_history>>_maxSalience>>_howGenerative;
	_probs.read(is);
	_fset.read(is);
}

void NaiveMaxEntEGrid::print(ostream& os)
{
// 	intProbMap& weights = _egProbs.weights();
// 	foreach(intProbMap, feat, weights)
// 	{
// 		os<<_fset.inv(feat->first)<<"\t"<<feat->second<<"\n";
// 	}

	os<<"Events following blank context (sal 2)\n";
	MaxEntContext context;
	for(int possProduced = T_SUBJ; 
		possProduced <= T_NONE;
		++possProduced)
	{
		//convert to 0-index
		int possProducedInd = possProduced - T_SUBJ;

		Feats feats;
		//prior
		int fnum;
		fnum = _fset.num(0, possProducedInd);
		if(fnum != -1)
		{
			feats.push_back(Feat(fnum, 1.0));
		}

		//history
		for(int ii = 0; ii < _history; ++ii)
		{
			//feats 1-k are history feats
			//the last one is salience
			int hi = T_NONE;

			fnum = _fset.pair(_fset.num(0, possProducedInd),
							  _fset.num(ii, hi));
			cerr<<"\t"<<_fset.inv(fnum)<<" "<<_probs.weights()[fnum]<<"\n";
			if(fnum != -1)
			{
				feats.push_back(Feat(fnum, 1.0));
			}
		}
		fnum = _fset.pair(_fset.num(0, possProducedInd),
						  _fset.num(_history, 2));

		context.push_back(feats);
	}
	for(int possProduced = T_SUBJ; 
		possProduced <= T_NONE;
		++possProduced)
	{
		os<<"P "<<NP::roleToString(possProduced)<<" "<<
			_probs.probToken(possProduced, context)<<"\n";
	}

	os<<"Events following subj immediately previous context (sal 2)\n";
	context.clear();
	for(int possProduced = T_SUBJ; 
		possProduced <= T_NONE;
		++possProduced)
	{
		//convert to 0-index
		int possProducedInd = possProduced - T_SUBJ;

		Feats feats;
		//prior
		int fnum;
		fnum = _fset.num(0, possProducedInd);
		if(fnum != -1)
		{
			feats.push_back(Feat(fnum, 1.0));
		}

		//history
		for(int ii = 0; ii < _history; ++ii)
		{
			//feats 1-k are history feats
			//the last one is salience
			int hi = T_NONE;
 			if(ii == _history - 1)
 			{
				hi = T_SUBJ;
			}

			fnum = _fset.pair(_fset.num(0, possProducedInd),
							  _fset.num(ii, hi));
			cerr<<"\t"<<_fset.inv(fnum)<<" "<<_probs.weights()[fnum]<<"\n";
			if(fnum != -1)
			{
				feats.push_back(Feat(fnum, 1.0));
			}
		}
		fnum = _fset.pair(_fset.num(0, possProducedInd),
						  _fset.num(_history, 2));

		context.push_back(feats);
	}
	for(int possProduced = T_SUBJ; 
		possProduced <= T_NONE;
		++possProduced)
	{
		os<<"P "<<NP::roleToString(possProduced)<<" "<<
			_probs.probToken(possProduced, context)<<"\n";
	}

	for(int feat = 0; feat < _probs.dimension(); ++feat)
	{
		if(_probs.weights()[feat] != 0)
		{
			os<<_fset.inv(feat)<<"\t"<<_probs.weights()[feat]<<"\n";
		}
	}
}

//log probability of a permuted document
Prob NaiveMaxEntEGrid::permProbability(Document& doc, ints& perm, bool train)
{
	_sentScores.clear();
	_sentScores.resize(perm.size());
	_globalScore = 0;

	int terms = 0;
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

//		cerr<<SYMTAB->toString(entity->first)<<"\n";

		int sent = 0;
		for(ints::iterator s = perm.begin();
			s != perm.end();
			s++,++sent)
		{
			int produced = _roles[entity->first][*s];

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

				MaxEntContext context;
				for(int possProduced = T_SUBJ; 
					possProduced <= T_NONE;
					++possProduced)
				{
					//convert to 0-index
					int possProducedInd = possProduced - T_SUBJ;

					Feats feats;

					//features are named (token produced x feature)

					//prior
					int fnum;
					fnum = _fset.num(0, possProducedInd);
					if(fnum != -1)
					{
						feats.push_back(Feat(fnum, 1.0));
					}

					//history
					int ii = 0;
					foreach(History, histItem, hist)
					{
						 //feats 1-k are history feats
						//the last one is salience
						++ii;

						//correct for new T_VERB between T_NONE and T_START
						int hi = (*histItem == T_START)?
							(T_NONE + 1):*histItem;

						fnum = _fset.pair(_fset.num(0, possProducedInd),
										  _fset.num(ii, hi));
						//cerr<<"\t"<<_fset.inv(fnum)<<"\n";
						if(fnum != -1)
						{
							feats.push_back(Feat(fnum, 1.0));
						}
					}
					//cerr<<"\n";

// 					History::iterator histItem = hist.begin();
// 					int prevItem = *histItem;
// 					++histItem;
// 					for(ii = 1; ii < _history; ++ii, ++histItem)
// 					{
// 						fnum = _fset.triple(_fset.num(0, possProducedInd),
// 											_fset.num(ii, prevItem),
// 											_fset.num(ii + 1, *histItem));
// 						prevItem = *histItem;
// 						//cerr<<"\t"<<_fset.inv(fnum)<<"\n";
// 						if(fnum != -1)
// 						{
// 							feats.push_back(Feat(fnum, 1.0));
// 						}
// 					}

					context.push_back(feats);
				}

				if(0)
				{
					cerr<<"\n";
					cerr<<produced<<" ";
					foreach(MaxEntContext, fv, context)
					{
						foreach(Feats, feat, *fv)
						{
							cerr<<_fset.inv(feat->first)<<
								":"<<feat->second<<":"<<
								_probs.weights()[feat->first]<<" ";
						}
					}
					cerr<<"\n";
				}

				if(train)
				{
					_probs.addCount(produced, context, 1.0);
				}
				else
				{
					Prob term = log(_probs.probToken(produced, context));
// 					cerr<<NP::roleToString(produced)<<" "<<
// 						_probs.probToken(produced, context)<<" "<<term<<"\n";
					res += term;
					_sentScores[sent] += term;
					terms += 1;
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
//allow us to check only part of the document?
// 		if(totalOcc != occSoFar)
// 		{
// 			cerr<<"Occurrence mismatch: "<<SYMTAB->toString(entity->first)<<
// 				" "<<totalOcc<<" cached occurrences, "<<
// 				occSoFar<<" counted occurrences.\n";
// 		}
//		assert(totalOcc == occSoFar);

		//original system also produces an end marker... this is really
		//not generative, since it 'generates' the document length
		//independently for each entity
		//we will encode the end marker as T_START
		if(_howGenerative == NOT_GEN)
		{
			cerr<<"Not supported.\n";
			assert(0);
		}
	}

	if(train)
	{
		return log(0); //makes it harder to use train mode by mistake!
	}
//	cerr<<"Terms "<<terms<<"\n";
	return res;
}

void NaiveMaxEntEGrid::addSalience(int occSoFar, int totalOcc, History& hist)
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
void NaiveMaxEntEGrid::estimate()
{
//	cerr<<"Learning rate "<<_egProbs.rate()<<"\n";
	_probs.setDimension(_fset.maxFeat());
	_probs.estimate(true);
}

void NaiveMaxEntEGrid::writeFeatures(bool on)
{
	_probs.writeFeatures(on);
}

void NaiveMaxEntEGrid::finishFeatures()
{
}

Prob NaiveMaxEntEGrid::readFeatureDump(string dump)
{
	_probs.readFeatures(dump);
	return 0;
}
