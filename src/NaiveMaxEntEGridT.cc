#include "NaiveMaxEntEGridT.h"

NaiveMaxEntEGridT::NaiveMaxEntEGridT(int hist, int maxSal, 
									 int howGenerative,
									 string ldaFile):
	NaiveMaxEntEGrid(hist, maxSal, howGenerative),
	_ldaFile(ldaFile)
{
	readExternalData();

	_fset.clear();
	int nProduced = T_VERB - T_SUBJ + 1;
	_fset.addType(string("produced"), nProduced);
	for(int ii = 0; ii < hist; ++ii)
	{
		//one more for T_START
		_fset.addType(string("hist")+intToString(ii), nProduced + 1);
	}
	_fset.addType(string("salience"), maxSal + 1);

	_fset.addType(string("verb"), 2);

//	_fset.addType(string("quantile"), 16);

// 	int topics = _lda->topics();
// 	for(int tt = 0; tt < topics; ++tt)
// 	{
// 		_fset.addType(string("topic") + intToString(tt), 2);
// 	}
}

NaiveMaxEntEGridT::NaiveMaxEntEGridT(istream& is):
	NaiveMaxEntEGrid(0, 0, 0)
{
	read(is);
}

void NaiveMaxEntEGridT::read(istream& is)
{
	NaiveMaxEntEGrid::read(is);
	_vocab.read(is);
	is>>_ldaFile;
	readExternalData();
}

void NaiveMaxEntEGridT::write(ostream& os)
{
	NaiveMaxEntEGrid::write(os);
	_vocab.write(os);
	os<<_ldaFile<<"\n";
}

void NaiveMaxEntEGridT::print(ostream& os)
{
	assert(_probs.dimension() == _vocab.size());
	for(int feat = 0; feat < _vocab.size(); ++feat)
	{
		unsigned int in = _vocab.inv(feat);
		os<<_fset.inv(in)<<"\t"<<_probs.weights()[feat]<<"\n";
	}
}

void NaiveMaxEntEGridT::readExternalData()
{
	ifstream ff(_ldaFile.c_str());
	assert(ff.is_open());
	_lda = new LDA(ff);
}

void NaiveMaxEntEGridT::initCaches(Document& doc)
{
	NaiveMaxEntEGrid::initCaches(doc);

	int size = doc.size();
	_cache.resize(size);
	int topics = _lda->topics();

	for(int ss = 0; ss < size; ++ss)
	{
		Sent* curr = doc[ss];
		ProbMat& lineCache = _cache[ss];
		lineCache.resize(T_NONE + 1);
		for(int ii = T_SUBJ; ii <= T_NONE; ++ii)
		{
			lineCache[ii].resize(topics);
		}

		foreach(NPs, np, curr->nps())
		{
			if((*np)->pronoun())
			{
				continue;
			}

//NO SYNTAX
			Probs& roleCache = lineCache[(*np)->role()];
//			Probs& roleCache = lineCache[T_SUBJ];

			Probs wordTopics;
			_lda->projectWord(stem((*np)->head()), wordTopics);

			for(int tt = 0; tt < topics; ++tt)
			{
				roleCache[tt] += wordTopics[tt];
			}

			if(_nounVectors.find((*np)->headSym()) == _nounVectors.end())
			{
				_nounVectors[(*np)->headSym()] = wordTopics;
			}
		}

		Trees leaves;
		Sent::getLeaves(curr->tree(), leaves);
		foreach(Trees, leaf, leaves)
		{
			char type = Sent::tag(*leaf)[0];
			if(type == 'V')
			{
				string word = stem(*leaf);
				if(word[0] == '\'')
				{
					continue;
				}

//NO SYNTAX
				int role = T_NONE;
//				int role = T_SUBJ;
				Probs& roleCache = lineCache[role];

				Probs wordTopics;
				_lda->projectWord(stem(*leaf), wordTopics);

				for(int tt = 0; tt < topics; ++tt)
				{
					roleCache[tt] += wordTopics[tt];
				}

				int skey = SYMTAB->toIndex(word);

				if(_roles.find(skey) == _roles.end())
				{
					int docLen = doc.size();
					for(int i=0; i<docLen; i++)
					{
						_roles[skey][i] = T_NONE;
					}
				}
				_roles[skey][ss] = T_VERB;

				if(_nounVectors.find(skey) == _nounVectors.end())
				{
					_nounVectors[skey] = wordTopics;
				}
			}
		}

		//norm?
// 		for(int role = T_SUBJ; role <= T_X; ++role)
// 		{
// 			Probs& roleCache = lineCache[role];

// 			Prob norm = 0;
// 			for(int tt = 0; tt < topics; ++tt)
// 			{
// 				norm += roleCache[tt];
// 			}
// 			for(int tt = 0; tt < topics; ++tt)
// 			{
// 				roleCache[tt] /= norm;
// 			}
// 		}
	}

	_occurrences.clear();
	cacheOccurrences(doc);
}

void NaiveMaxEntEGridT::clearCaches()
{
	NaiveMaxEntEGrid::clearCaches();
	_cache.clear();
	_nounVectors.clear();
}

Prob NaiveMaxEntEGridT::permProbability(Document& doc, ints& perm, bool train)
{
	_sentScores.clear();
	_sentScores.resize(perm.size());
	_globalScore = 0;

	Prob res = 0.0;

	int topics = _lda->topics();

	int size = perm.size();
	for(int sent = 0; sent < size; ++sent)
	{
		int permInd = perm[sent];

		for(symToIntToInt::iterator entity = _roles.begin();
			entity != _roles.end();
			entity++)
		{
			int produced = _roles[entity->first][permInd];

			assert(inMap(_nounVectors, entity->first));
			Probs& wordTopics = _nounVectors[entity->first];

			MaxEntContext context;
			for(int possProduced = T_SUBJ; 
				possProduced <= T_VERB;
				++possProduced)
			{
				Feats feats;

				int fnum;
				fnum = _vocab.get(_fset.num(0, possProduced), train);
				if(fnum != -1)
				{
					feats.push_back(Feat(fnum, 1.0));
				}

				//no NP objects associated with verbs
				bool isVerb = _reps[entity->first].empty();
				fnum = _vocab.get(
					_fset.pair(_fset.num(0, possProduced),
							   _fset.num(_history + 2, isVerb)),
					train);
				if(fnum != -1)
				{
					feats.push_back(Feat(fnum, 1.0));
				}				

				int totalOcc = _occurrences[entity->first];
				if(totalOcc == 0)
				{
					cerr<<SYMTAB->toString(entity->first)<<
						" occurs, but does not occur\n";
				}
				assert(totalOcc != 0);

				fnum = _vocab.get(
					_fset.pair(_fset.num(0, possProduced),
							   _fset.num(_history + 1, totalOcc)),
					train);
				if(fnum != -1)
				{
					feats.push_back(Feat(fnum, 1.0));
				}

				for(int prev = 0; prev < _history; ++prev)
				{
					int prevSI = sent - (prev + 1);
					if(prevSI < 0)
					{
						fnum = _vocab.get(
							_fset.pair(_fset.num(0, possProduced),
									   _fset.num(prev + 1, T_START)),
							train);

						if(fnum != -1)
						{
							feats.push_back(Feat(fnum, 1.0));
						}
					}
					else
					{
						int prevPermInd = perm[sent - (prev + 1)];
						ProbMat& prevCache = _cache[prevPermInd];
						for(int role = T_SUBJ; role <= T_NONE; ++role)
						{
							Probs& roleCache = prevCache[role];
							Prob dot = 0;

							for(int topic = 0; topic < topics; ++topic)
							{
								//max-of-mins
								Prob term = roleCache[topic];
								if(wordTopics[topic] < roleCache[topic])
								{
									term = wordTopics[topic];
								}
								if(term > dot)
								{
									dot = term;
								}

								//dot product
// 								Prob term = 
// 									roleCache[topic] * wordTopics[topic];
// 								dot += term;

								//topic-wise entries
// 								if(term < 1e-5)
// 								{
// 									continue;
// 								}

// 								term = log(term);

// 								fnum = _vocab.get(
// 									_fset.triple(
// 									_fset.num(0, possProduced),
// 									_fset.num(prev + 1, role),
// 									_fset.num(_history + 2 + topic, 1)),
// 									train);
// 								if(fnum != -1)
// 								{
// 									feats.push_back(Feat(fnum, term));
// 								}
							}

							if(dot < 10./topics)
							{
								continue;
							}

							//transforms
//							dot = log(dot);

							//threshold (isn't good)
// 							if(dot > 10.0/topics)
// 							{
// 								dot = 1;
// 							}
// 							else
// 							{
// 								dot = 0;
// 							}

							//finer threshold
// 							Prob quantized = (int)(15 * dot);

// 							fnum = _vocab.get(
// 								_fset.triple(
// 									_fset.num(0, possProduced),
// 									_fset.num(prev + 1, role),
// 									_fset.num(_history + 2, quantized)),
// 								train);
// 							if(fnum != -1)
// 							{
// 								feats.push_back(Feat(fnum, 1.0));
// 							}

							fnum = _vocab.get(
								_fset.pair(
									_fset.num(0, possProduced),
									_fset.num(prev + 1, role)),
								train);
							if(fnum != -1)
							{
								feats.push_back(Feat(fnum, dot));
							}
						}
					}
				}

				context.push_back(feats);
			}

			//prob block
			if(train)
			{
				_probs.addCount(produced, context, 1.0);
			}
			else
			{
				Prob term = log(_probs.probToken(produced, context));
				if(term > 0 || !isfinite(term))
				{
					term = log(1e-10);
				}
				assertLogProb(term);
				res += term;
				_sentScores[sent] += term;
			}
		}
	}

	if(train)
	{
		return log(0); //makes it harder to use train mode by mistake!
	}
	return res;
}

void NaiveMaxEntEGridT::estimate()
{
	_probs.setDimension(_vocab.size());
	_probs.estimate(true);
}


void NaiveMaxEntEGridT::finishFeatures()
{
	boost::iostreams::filtering_ostream* spool = _probs._spool;
	(*spool)<<"-3\n";
	_vocab.write(*spool);
}

Prob NaiveMaxEntEGridT::readFeatureDump(string dump)
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

	intIntMap transTab;
	_vocab.translate(dumpVocab, transTab);

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
	}

	return 0;
}
