#include "NaiveMaxEntEGridF.h"
#include "treeInfo.h"

//history size, max salience level, smoothing (additive)
//generativity level
// = NOT -- assigns probability to production after eg [S S 2]
// = SLIGHTLY -- no end marker
// = MORE -- no probability for production after [S S 2]
// = REALLY -- salience represents previously-observed occurrence counts
NaiveMaxEntEGridF::NaiveMaxEntEGridF(int hist, int maxSal, 
								   int howGenerative):
	NaiveMaxEntEGrid(hist, maxSal, howGenerative)
{
	F_PRIOR = _fset.find(string("produced"));
	F_HIST = _fset.addType(string("history"), numContextTypes());
 	F_LINK = _fset.addType(string("link"), 2);
 	F_UNLINK = _fset.addType(string("unlink"), 2);
 	F_PROPER = _fset.addType(string("proper"), 2);
	F_NE = _fset.addType(string("namedent"), NE_MNY + 1);
	F_MOD = _fset.addType(string("mod"), 4);
	F_PLUR = _fset.addType(string("plural"), 2);
	F_HASPRO = _fset.addType(string("haspro"), 2);
	F_NEVERPRO = _fset.addType(string("neverpro"), 2);
	F_RELCLAUSE = _fset.addType(string("rel"), 2);
	F_HEAD = _fset.addType(string("head"), 2);

//	cerr<<"fns "<<F_PRIOR<<" "<<F_HIST<<" "<<F_LINK<<"\n";

	readExternalData();
}

NaiveMaxEntEGridF::NaiveMaxEntEGridF(istream& is):
	NaiveMaxEntEGrid(0, 0, 0)
{
	read(is);

	readExternalData();
}

void NaiveMaxEntEGridF::write(ostream& os)
{
	NaiveMaxEntEGrid::write(os);
	_vocab.write(os);
}

//initializes this class -- called by the istream constructor
void NaiveMaxEntEGridF::read(istream& is)
{
	NaiveMaxEntEGrid::read(is);
	_vocab.read(is);

	F_PRIOR = _fset.find(string("produced"));
	F_HIST = _fset.find(string("history"));
	F_LINK = _fset.find(string("link"));
	F_UNLINK = _fset.find(string("unlink"));
	F_PROPER = _fset.find(string("proper"));
	F_NE = _fset.find(string("namedent"));
	F_MOD = _fset.find(string("mod"));
	F_PLUR = _fset.find(string("plural"));
	F_HASPRO = _fset.find(string("haspro"));
	F_NEVERPRO = _fset.find(string("neverpro"));
	F_RELCLAUSE = _fset.find(string("rel"));
	F_HEAD = _fset.find(string("head"));
}

void NaiveMaxEntEGridF::readExternalData()
{
	string unlink = DATA_PATH + "/unlinkable";
	ifstream ff(unlink.c_str());
	assert(ff.is_open());
	string key;
	while(ff>>key)
	{
		_unlinkable.insert(key);
	}

	string link = DATA_PATH + "/train-linkable";
	ifstream ff2(link.c_str());
	assert(ff2.is_open());
	strIntMap mm;
	readHashMap(mm, ff2);
	foreach(strIntMap, it, mm)
	{
		_linkable.insert(it->first);
	}

	string pro = DATA_PATH + "/proStats.txt";
	ifstream ff3(pro.c_str());
	assert(ff3.is_open());
	readSet(_unPro, ff3);
	readSet(_hasPro, ff3);	
}

void NaiveMaxEntEGridF::print(ostream& os)
{
	assert(_probs.dimension() == _vocab.size());
	for(int feat = 0; feat < _vocab.size(); ++feat)
	{
		unsigned int in = _vocab.inv(feat);
		os<<_fset.inv(in)<<"\t"<<_probs.weights()[feat]<<"\n";
	}
}

//log probability of a permuted document
Prob NaiveMaxEntEGridF::permProbability(Document& doc, ints& perm, bool train)
{
	_sentScores.clear();
	_sentScores.resize(perm.size());
	_globalScore = 0;

	Prob res = 0.0;

	for(symToIntToInt::iterator entity = _roles.begin();
		entity != _roles.end();
		entity++)
	{
		int occSoFar = 0;
		int totalOcc = _occurrences[entity->first];
		History hist;

		//keep track of the last pair of context terms
		int shortLen = (_history < 2) ? _history : 2;
		History shortHist;

		for(int i = 0; i < _history; i++)
		{
			hist.push_back(T_START);
		}

		for(int i = 0; i < shortLen; i++)
		{
			shortHist.push_back(T_START);
		}

		int sent = 0;
		for(ints::iterator s = perm.begin();
			s != perm.end();
			s++,sent++)
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

				addSalience(occSoFar, totalOcc, shortHist);
				int histNum = histToNum(shortHist, shortLen);

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
					fnum = _vocab.get(_fset.num(F_PRIOR, possProducedInd), 
									  train);
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

						int hi = (*histItem == T_START)?
							(T_NONE + 1):*histItem;

						fnum = _vocab.get(
							_fset.pair(_fset.num(F_PRIOR, possProducedInd),
									   _fset.num(ii, hi)),
							train);
						//cerr<<"\t"<<_fset.inv(fnum)<<"\n";
						if(fnum != -1)
						{
							feats.push_back(Feat(fnum, 1.0));
						}
					}

					//full history
					fnum = _vocab.get(
						_fset.pair(_fset.num(F_PRIOR, possProducedInd),
								   _fset.num(F_HIST, histNum)),
						train);
					if(fnum != -1)
					{
						feats.push_back(Feat(fnum, 1.0));
					}

					//could this be cheating?
//					NP* rep = _reps[entity->first].front();
					int randIndex = 
						gsl_rng_uniform_int(RNG, 
											_reps[entity->first].size());
					NP* rep = _reps[entity->first][randIndex];

					bool isUnlinkable = contains(_unlinkable, rep->head());

					bool isLinkable = contains(_linkable, rep->head());

					bool isProper = rep->proper();

					int neType = rep->namedEntityType();

					bool plur = plr(rep->node());

					bool unPro = contains(_unPro, rep->head());

					bool hasPro = contains(_hasPro, rep->head());

					bool head = rep->discNewMarkable();

					int numMods = 0;
					bool rel = false;
					foreach(NPs, np, _reps[entity->first])
					{
						//the tagger is mostly, but not always, consistent
						//cerr<<NP::neToString((*np)->namedEntityType())<<" ";

						Trees mods;
						Sent::getModifiers((*np)->node(), mods, mods);
						foreach(Trees, modTree, mods)
						{
							numMods++;
							if((*modTree)->label == sbarLabel)
							{
								rel = true;
								//cerr<<(**np)<<"\n";
							}
						}
					}
					//cerr<<"\n";

					if(numMods > 10)
					{
						numMods = 3;
					}
					else if(numMods > 5)
					{
						numMods = 2;
					}
					else if(numMods > 0)
					{
						numMods = 1;
					}

					fnum = _vocab.get(
						_fset.triple(_fset.num(F_PRIOR, possProducedInd),
									 _fset.num(F_HIST, histNum),
									 _fset.num(F_UNLINK, isUnlinkable)),
						train);
					if(fnum != -1)
					{
						feats.push_back(Feat(fnum, 1.0));
					}

					fnum = _vocab.get(
						_fset.triple(_fset.num(F_PRIOR, possProducedInd),
									 _fset.num(F_HIST, histNum),
									 _fset.num(F_LINK, isLinkable)),
						train);
					if(fnum != -1)
					{
						feats.push_back(Feat(fnum, 1.0));
					}

					fnum = _vocab.get(
						_fset.triple(_fset.num(F_PRIOR, possProducedInd),
									 _fset.num(F_HIST, histNum),
									 _fset.num(F_PROPER, isProper)),
						train);
					if(fnum != -1)
					{
						feats.push_back(Feat(fnum, 1.0));
					}

					fnum = _vocab.get(
						_fset.triple(_fset.num(F_PRIOR, possProducedInd),
									 _fset.num(F_HIST, histNum),
									 _fset.num(F_NE, neType)),
						train);
					if(fnum != -1)
					{
						feats.push_back(Feat(fnum, 1.0));
					}

					fnum = _vocab.get(
						_fset.triple(_fset.num(F_PRIOR, possProducedInd),
									 _fset.num(F_HIST, histNum),
									 _fset.num(F_PLUR, plur)),
						train);
					if(fnum != -1)
					{
						feats.push_back(Feat(fnum, 1.0));
					}

					fnum = _vocab.get(
						_fset.triple(_fset.num(F_PRIOR, possProducedInd),
									 _fset.num(F_HIST, histNum),
									 _fset.num(F_NEVERPRO, unPro)),
						train);
					if(fnum != -1)
					{
						feats.push_back(Feat(fnum, 1.0));
					}

					fnum = _vocab.get(
						_fset.triple(_fset.num(F_PRIOR, possProducedInd),
									 _fset.num(F_HIST, histNum),
									 _fset.num(F_HASPRO, hasPro)),
						train);
					if(fnum != -1)
					{
						feats.push_back(Feat(fnum, 1.0));
					}

					fnum = _vocab.get(
						_fset.triple(_fset.num(F_PRIOR, possProducedInd),
									 _fset.num(F_HIST, histNum),
									 _fset.num(F_MOD, numMods)),
						train);
					if(fnum != -1)
					{					   
						feats.push_back(Feat(fnum, 1.0));
					}

					fnum = _vocab.get(
						_fset.triple(_fset.num(F_PRIOR, possProducedInd),
									 _fset.num(F_HIST, histNum),
									 _fset.num(F_HEAD, head)),
						train);
					if(fnum != -1)
					{					   
						feats.push_back(Feat(fnum, 1.0));
					}

// 					cerr<<"Unlinkable "<<isUnlinkable<<"\n";
// 					cerr<<"Linkable "<<isLinkable<<"\n";
// 					cerr<<"Proper "<<isProper<<"\n";
// 					cerr<<"NE type "<<neType<<"\n";
// 					cerr<<"Plural "<<plur<<"\n";
// 					cerr<<"Unpro "<<unPro<<"\n";
// 					cerr<<"Has pro "<<hasPro<<"\n";
// 					cerr<<"Num mods "<<numMods<<"\n";
// 					cerr<<"Head "<<head<<"\n";

					context.push_back(feats);
				}

				if(train)
				{
					_probs.addCount(produced, context, 1.0);
				}
				else
				{
// 					cerr<<"Prob of "<<NP::roleToString(produced)<<" is "<<
// 						_probs.probToken(produced, context)<<"\n";

					Prob term = log(_probs.probToken(produced, context));
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

			shortHist.pop_back(); //boot out the salience
			shortHist.pop_front(); //and the oldest context item
			shortHist.push_back(produced); //stick in the newest context item
		}
		if(totalOcc != occSoFar)
		{
			cerr<<"Occurrence mismatch: "<<SYMTAB->toString(entity->first)<<
				" "<<totalOcc<<" cached occurrences, "<<
				occSoFar<<" counted occurrences.\n";
		}
		assert(totalOcc == occSoFar);

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
	return res;
}

//estimate parameters
void NaiveMaxEntEGridF::estimate()
{
	_probs.setDimension(_vocab.size());
	_probs.estimate(true);
}

void NaiveMaxEntEGridF::finishFeatures()
{
	boost::iostreams::filtering_ostream* spool = _probs._spool;
	(*spool)<<"-3\n";
	_vocab.write(*spool);
}

Prob NaiveMaxEntEGridF::readFeatureDump(string dump)
{
	MaxEntSelection dummy(0);

	boost::iostreams::filtering_istream ifs;
    ifs.push(boost::iostreams::bzip2_decompressor());
    ifs.push(boost::iostreams::file_source(dump));

	cerr<<"Opening trace file "<<dump.c_str()<<"\n";
	assert(!ifs.fail());

	dummy.readFeatures(ifs);

	IntVocab dumpVocab;
	dumpVocab.read(ifs);
	cerr<<"Read vocab.\n";

	intIntMap transTab;
	_vocab.translate(dumpVocab, transTab);
	cerr<<"Translation table complete.\n";

	int ctr = 0;
	foreach(Examples, exe, dummy._examples)
	{
		MaxEntContext myCtxt;
		foreach(MaxEntContext, fv, *(*exe)->context)
		{
			Feats myFv;
			foreach(Feats, feat, *fv)
			{
// 				cerr<<feat->first<<" "<<dumpVocab.inv(feat->first)<<" "<<
// 					_fset.inv(dumpVocab.inv(feat->first))<<"\n";
				myFv.push_back(Feat(transTab[feat->first], feat->second));
			}
			myCtxt.push_back(myFv);
		}

		Example* myExe;
		_probs._examples.push_back(new Example(myCtxt));
		myExe = _probs._examples.back();
		myExe->counts = (*exe)->counts;

		delete *exe;

		++ctr;

		if(ctr % 10000 == 0)
		{
			cerr<<"Translated "<<ctr<<"...\n";
		}
	}

	dummy._examples.clear(); //in case destructor wants to clear them

	return 0;
}
