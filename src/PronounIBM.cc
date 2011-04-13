#include "PronounIBM.h"
#include "treeInfo.h"
#include "Features.h"

PronounIBM::PronounIBM(int prevSents):
	ECIBM()
{
	_prevSents = prevSents;
	_alignment.setDimension(Features::maxFeat());

	string apath = DATA_PATH + "/animacy.txt";
	ifstream lexStats(apath.c_str());
	assert(lexStats.is_open());
	readLexicalStatistics(lexStats);
}

PronounIBM::PronounIBM(istream& in):
	ECIBM()
{
	read(in);
}

PronounIBM::PronounIBM(IBM& other):
	ECIBM(other)
{
	_prevSents = other._prevSents;

	PronounIBM* oo = dynamic_cast<PronounIBM*>(&other);
	if(oo)
	{
		_nonAna = oo->_nonAna;
	}

	for(int i = 0; i < _vocab.size(); ++i)
	{
		_produced.insert(i);
	}

	_estimating = false;

    if(_animacy.empty())
	{
		string apath = DATA_PATH + "/animacy.txt";
		ifstream lexStats(apath.c_str());
		assert(lexStats.is_open());
		readLexicalStatistics(lexStats);
	}
}

void PronounIBM::read(istream& is)
{
	ECIBM::read(is);
	readSet(_nonAna, is);
}

void PronounIBM::write(ostream& os)
{
	ECIBM::write(os);
	writeSet(_nonAna, os);
}

void PronounIBM::print(ostream& os)
{
//	TopicIBM::print(os);

	int size = Features::maxFeat();
	Prob* weights = _alignment.weights();
	for(int i = 0; i < size; ++i)
	{
		if(weights[i] != 0)
		{
			os<<Features::inv(i)<<":\t"<<weights[i]<<"\n";
		}
	}
}

int PronounIBM::history()
{
	return _prevSents;
}

void PronounIBM::initCaches(Document& doc)
{
	ECIBM::initCaches(doc);

	symToNPs& head = doc.byHead();
	foreach(symToNPs, sym, head)
	{
		int key = _vocab.get(sym->first, true);
		int occ = sym->second.size();
		//XXX turn off cap on freq counting
// 		if(occ > 3)
// 		{
// 			occ = 3;
// 		}
		if(sym->second[0]->pronoun())
		{
			occ = 0;
		}
//		cerr<<SYMTAB->toString(sym->first)<<" "<<occ<<"\n";
		_occurrences[key] = occ;
	}
}

void PronounIBM::clearCaches()
{
	ECIBM::clearCaches();
	_occurrences.clear();
}

void PronounIBM::featurizeDistances(NounContext& local, NounContexts& global,
									bool add)
{
	//rather than actually make sure that references are sorted so that
	//the closest are encountered first, we will assume this happens
	//naturally... this is, however, not true in the case of possessives
	//which always occur last in the context vector
	//--but they are such terrible antecedents we will simply ignore them
	int prevAcceptableRefs = 0;
	int prevProperRefs = 0;

	revforeach(NounContext, word, local)
	{
		if(word->word >= _nulls)
		{
			word->clearDistFeats();

			int dpth = Sent::depth(word->tree, sLabel);
			int govTyp = govType(word->tree);
			int isAnc = Sent::isAncestor(local.tree, word->tree);

			int sameClause = 0;
			Tree* wSent = Sent::ancestor(local.tree, sLabel);
			Tree* anteSent = Sent::ancestor(word->tree, sLabel);
			if(wSent && wSent == anteSent)
			{
				sameClause = 1;
			}

			Tree* common = NULL;
			for(Tree* testingAnc = anteSent;
				testingAnc != NULL;
				testingAnc = Sent::ancestor(testingAnc, sLabel))
			{
				for(Tree* testingWAnc = wSent;
					testingWAnc != NULL;
					testingWAnc = Sent::ancestor(testingWAnc, sLabel))
				{
					if(testingWAnc == testingAnc)
					{
						common = testingWAnc;
					}
				}
			}
			if(common)
			{
				dpth -= Sent::depth(common, sLabel);
			}
			if(dpth > 2)
			{
				dpth = 2;
			}

			int refDist = prevAcceptableRefs;
			int properRefDist = 0;
			if(proType(word->tree) != 2) //possessive
			{
				if(prevAcceptableRefs < 5)
				{
					prevAcceptableRefs++;
				}
			}
			if(Sent::proper(Sent::head(word->tree)->label))
			{
				properRefDist = prevProperRefs;
				if(prevProperRefs < 2)
				{
					prevProperRefs++;
				}
			}

			Prob animacy = ifExists(_animacy, word->word, .5);
			animacy *= (F_LIMITS[F_LEX_ANIM] - 1);
			int lexAnimacy = static_cast<int>(animacy);
			if(lexAnimacy == F_LIMITS[F_LEX_ANIM])
			{
				//happens when underlying p = 1
				lexAnimacy -= 1;
			}

			Prob freq = ifExists(_frequency, word->word, 1e-10);
			Prob logFreq = logl(freq * 100);
			int wordFreq = int(-logFreq);
			if(wordFreq < 0)
			{
				wordFreq = 0;
			}
			if(wordFreq >= F_LIMITS[F_LEX_FREQ])
			{
				wordFreq = F_LIMITS[F_LEX_FREQ] - 1;
			}

// 			cerr<<"word "<<Sent::headWord(word->tree)<<" "<<
// 				animacy<<" "<<lexAnimacy<<"\n";

			ints anteECFeats;
			anteTabIndex(local, *word, anteECFeats);
			anteECFeats.push_back(dpth);
			anteECFeats.push_back(govTyp);
			anteECFeats.push_back(isAnc);
			anteECFeats.push_back(sameClause);
			anteECFeats.push_back(refDist);
			anteECFeats.push_back(properRefDist);
			anteECFeats.push_back(lexAnimacy);
			anteECFeats.push_back(wordFreq);

			distFeatsPostprocess(*word, anteECFeats);

// 			string fullDistFeat = "ecbucket"+intToString(atx);
// 			int fname = _alignmentFeatures.get(fullDistFeat, add);
// 			if(fname != -1)
// 			{
// 				word->addDistFeat(fname);
// 			}

			addCombinedFeats(*word, anteECFeats, add);
		}
	}

	revforeach(NounContexts, context, global)
	{
		foreach(NounContext, word, *context)
		{
			if(word->word >= _nulls)
			{
				word->clearDistFeats();

				int dpth = Sent::depth(word->tree, sLabel);
				if(dpth > 2)
				{
					dpth = 2;
				}
				int govTyp = govType(word->tree);
				int isAnc = Sent::isAncestor(local.tree, word->tree);
				int sameClause = 0; //not same ss

				int refDist = prevAcceptableRefs;
				int properRefDist = 0;
				if(proType(word->tree) != 2) //possessive
				{
					if(prevAcceptableRefs < 5)
					{
						prevAcceptableRefs++;
					}
				}
				if(Sent::proper(Sent::head(word->tree)->label))
				{
					properRefDist = prevProperRefs;
					if(prevProperRefs < 2)
					{
						prevProperRefs++;
					}
				}

				Prob animacy = ifExists(_animacy, word->word, .5);
				animacy *= (F_LIMITS[F_LEX_ANIM]);
				int lexAnimacy = static_cast<int>(animacy);
				if(lexAnimacy == F_LIMITS[F_LEX_ANIM])
				{
					//happens when underlying p = 1
					lexAnimacy -= 1;
				}

				Prob freq = ifExists(_frequency, word->word, 1e-10);
				Prob logFreq = logl(freq * 100);
				int wordFreq = int(-logFreq);
				if(wordFreq < 0)
				{
					wordFreq = 0;
				}
				if(wordFreq >= F_LIMITS[F_LEX_FREQ])
				{
					wordFreq = F_LIMITS[F_LEX_FREQ] - 1;
				}

// 				cerr<<"word "<<Sent::headWord(word->tree)<<" "<<
// 					animacy<<" "<<lexAnimacy<<"\n";

				ints anteECFeats;
				anteTabIndex(local, *word, anteECFeats);
				anteECFeats.push_back(dpth);
				anteECFeats.push_back(govTyp);
				anteECFeats.push_back(isAnc);
				anteECFeats.push_back(sameClause);
				anteECFeats.push_back(refDist);
				anteECFeats.push_back(properRefDist);
				anteECFeats.push_back(lexAnimacy);
				anteECFeats.push_back(wordFreq);

				distFeatsPostprocess(*word, anteECFeats);

// 				string fullDistFeat = "ecbucket"+intToString(atx);
// 				int fname = _alignmentFeatures.get(fullDistFeat, add);
// 				if(fname != -1)
// 				{
// 					word->addDistFeat(fname);
// 				}

				addCombinedFeats(*word, anteECFeats, add);
			}
		}
	}
}

#if 0
void PronounIBM::addCombinedFeats(ContextWord& word, ints& feats, int nC,
	bool add)
{
	if(nC == 0)
	{
		return;
	}

	strSet featNames;
	featNames.insert("syn"+intToString(feats[0]));
	featNames.insert("sent"+intToString(feats[1]));
	featNames.insert("wdist"+intToString(feats[2]));
	featNames.insert("proWordPos"+intToString(feats[3]));
	featNames.insert("proType"+intToString(feats[4]));
	featNames.insert("anteType"+intToString(feats[5]));
	featNames.insert("depth"+intToString(feats[6]));
	featNames.insert("gov"+intToString(feats[7]));
	featNames.insert("anc"+intToString(feats[8]));
	featNames.insert("sameclause"+intToString(feats[9]));

//	writeVector(feats, cerr);

	strSets allFeatNames;
	strSets newFeatNames;
	foreach(strSet, fn, featNames)
	{
		strSet container;
		container.insert(*fn);
		newFeatNames.insert(container);
	}

	allFeatNames = newFeatNames;

	for(int round = 1; round < nC; ++round)
	{
		foreach(strSet, fn, featNames)
		{
			foreach(strSets, feat, allFeatNames)
			{
				strSet newFeat = *feat;
				newFeat.insert(*fn);
				newFeatNames.insert(newFeat);
			}
		}
		allFeatNames = newFeatNames;
	}

// 	foreach(strSets, feat, allFeatNames)
// 	{
// 		foreach(strSet, ss, *feat)
// 		{
// 			cerr<<*ss<<" ";
// 		}
// 		cerr<<"\n";
// 	}

// 	cerr<<"---------------------\n";

	foreach(strSets, feat, allFeatNames)
	{
		ostringstream comboFeat;
		foreach(strSet, ss, *feat)
		{
			comboFeat<<*ss<<"-";
		}
		int fkey = _alignmentFeatures.get(comboFeat.str(), add);
		if(fkey != -1)
		{
			word.addDistFeat(fkey);
		}
	}
}

void PronounIBM::addCombinedFeats(ContextWord& word, ints& feats, int nC,
	bool add)
{
	assert(nC == 2); //this fn optimized for 2 only

	strings featNames(10);
	featNames[0] = "syn"+intToString(feats[0]);
	featNames[1] = "sent"+intToString(feats[1]);
	featNames[2] = "wdist"+intToString(feats[2]);
	featNames[3] = "proWordPos"+intToString(feats[3]);
	featNames[4] = "proType"+intToString(feats[4]);
	featNames[5] = "anteType"+intToString(feats[5]);
	featNames[6] = "depth"+intToString(feats[6]);
	featNames[7] = "gov"+intToString(feats[7]);
	featNames[8] = "anc"+intToString(feats[8]);
	featNames[9] = "sameclause"+intToString(feats[9]);

	foreach(strings, ff, featNames)
	{
		int fkey = _alignmentFeatures.get(*ff, add);
		if(fkey != -1)
		{
			word.addDistFeat(fkey);
		}
	}

	int size = featNames.size();
	for(int ii = 0; ii < size; ++ii)
	{
		for(int jj = 0; jj < ii; ++jj)
		{
			string fname = featNames[ii]+"-"+featNames[jj];
			int fkey = _alignmentFeatures.get(fname, add);
			if(fkey != -1)
			{
				word.addDistFeat(fkey);
 			}
		}
	}
}
#endif

#ifdef TRIPLE_FEATS

void PronounIBM::addCombinedFeats(ContextWord& word, ints& feats, bool add)
{
	int existingFeats = word.feats.size();

	int size = feats.size();
	for(int ii = 0; ii < size; ++ii)
	{
		if(feats[ii] == -1)
		{
			continue;
		}

//		cerr<<ii<<" "<<F_NAMES[ii + F_SYN]<<" "<<feats[ii]<<"\n";
		int fkey = Features::num(ii + F_SYN, feats[ii]);
		if(word.mode == 1)
		{
			fkey = Features::noncoref(fkey);
		}
		word.addDistFeat(fkey);
	}

	//quick hack: add features created with 'featurize' to the feats
	//vector so that the appropriate feature pairings are generated
	for(int ii = 1; ii < existingFeats; ++ii)
	{
		//don't try to pair up features that aren't part of the feature
		//type system
		if(word.feats[ii].first < Features::maxFeat())
		{
			int ftype;
			int fval;
			Features::fvalue(word.feats[ii].first, ftype, fval);
//			cerr<<"adding "<<word.feats[ii].first<<" "<<ftype<<" "<<F_NAMES[ftype]<<"\n";
			assert(feats.size() + F_SYN == ftype);
			feats.push_back(fval);
		}
	}
	size = feats.size();

	for(int ii = 0; ii < size; ++ii)
	{
		if(feats[ii] == -1)
		{
			continue;
		}

		int ifeat = Features::num(ii + F_SYN, feats[ii]);
// 		if(word.mode == 1)
// 		{
// 			ifeat = Features::noncoref(ifeat);
// 		}

		for(int jj = 0; jj < ii; ++jj)
		{
			if(feats[jj] == -1)
			{
				continue;
			}

			int jfeat = Features::num(jj + F_SYN, feats[jj]);
// 			if(word.mode == 1)
// 			{
// 				jfeat = Features::noncoref(jfeat);
// 			}

			int fkey = Features::pair(jfeat, ifeat);
	
			word.addDistFeat(fkey);

			for(int kk = 0; kk < jj; ++kk)
			{
				if(feats[kk] == -1)
				{
					continue;
				}

				int kfeat = Features::num(kk + F_SYN, feats[kk]);
// 				if(word.mode == 1)
// 				{
// 					kfeat = Features::noncoref(kfeat);
// 				}

				int fkey2 = Features::triple(kfeat, jfeat, ifeat);

				word.addDistFeat(fkey2);
			}
		}
	}

// 	cerr<<"------------------\n";
// 	foreach(Feats, feat, word.feats)
// 	{
// 		cerr<<Features::inv(feat->first)<<" ";
// 	}
// 	cerr<<"\n-----------------\n";
}

#else

void PronounIBM::addCombinedFeats(ContextWord& word, ints& feats, bool add)
{
	int existingFeats = word.feats.size();

	int size = feats.size();
	for(int ii = 0; ii < size; ++ii)
	{
		if(feats[ii] == -1)
		{
			continue;
		}

//		cerr<<ii<<" "<<F_NAMES[ii + F_SYN]<<" "<<feats[ii]<<"\n";
		int fkey = Features::num(ii + F_SYN, feats[ii]);
		if(word.mode == 1)
		{
			fkey = Features::noncoref(fkey);
		}
		word.addDistFeat(fkey);
	}

	//don't square the feat vector
	return;

	//quick hack: add features created with 'featurize' to the feats
	//vector so that the appropriate feature pairings are generated
	for(int ii = 1; ii < existingFeats; ++ii)
	{
		//don't try to pair up features that aren't part of the feature
		//type system
		if(word.feats[ii].first < Features::maxFeat())
		{
			int ftype;
			int fval;
			Features::fvalue(word.feats[ii].first, ftype, fval);
//			cerr<<"adding "<<word.feats[ii].first<<" "<<ftype<<" "<<F_NAMES[ftype]<<"\n";
			assert(feats.size() + F_SYN == ftype);
			feats.push_back(fval);
		}
	}
	size = feats.size();

	for(int ii = 0; ii < size; ++ii)
	{
		if(feats[ii] == -1)
		{
			continue;
		}

		int ifeat = Features::num(ii + F_SYN, feats[ii]);
		if(word.mode == 1)
		{
			ifeat = Features::noncoref(ifeat);
		}

		for(int jj = 0; jj < ii; ++jj)
		{
			if(feats[jj] == -1)
			{
				continue;
			}

			int jfeat = Features::num(jj + F_SYN, feats[jj]);
			if(word.mode == 1)
			{
				jfeat = Features::noncoref(jfeat);
			}

			int fkey = Features::pair(jfeat, ifeat);
	
			word.addDistFeat(fkey);
		}
	}

// 	cerr<<"------------------\n";
// 	foreach(Feats, feat, word.feats)
// 	{
// 		cerr<<Features::inv(feat->first)<<" ";
// 	}
// 	cerr<<"\n-----------------\n";
}

#endif

void PronounIBM::featurize(ContextWord& created, bool add)
{
	int md = modType(created.tree, NULL);
	int dt = determinerType(created.tree);
	int freqN = _occurrences[created.word];
	if(freqN >= F_LIMITS[F_FREQ])
	{
		freqN = F_LIMITS[F_FREQ] - 1;
	}

	int fkey = Features::num(F_MOD, md);
	created.feats.push_back(Feat(fkey, 1));
	fkey = Features::num(F_DET, dt);
	created.feats.push_back(Feat(fkey, 1));

	fkey = Features::num(F_FREQ, freqN);
	created.feats.push_back(Feat(fkey, 1.0));

    //note: feature here is continuous-valued
	//fkey = Features::num(F_FREQ, 0);
	//created.feats.push_back(Feat(fkey, freqN));

// 	cerr<<"featurized\n";
// 	for(int i=0; i<created.feats.size();++i)
// 	{
// 		cerr<<Features::inv(created.feats[i].first)<<"\n";
// 	}
// 	cerr<<"------\n";
}

Prob PronounIBM::wordGivenPrevWord(NounContext& next, ContextWord& prevWord, 
								   bool train,
								   Prob totAlign, MaxEntContext& context)
{
	Prob alignProb;
	if(train && !_estimating)
	{
		alignProb = 1.0/totAlign;

		//if(prevWord.word < _nulls)
		{
			getRandBias(prevWord.word, next.word);
		}
	}
	else
	{
		alignProb = _alignment.probToken(prevWord.index, context);
	}


	if(prevWord.word < _nulls)
	{
		//force non-ana decisions as memorized
		if(contains(_nonAna, next.np->fullName()))
		{
			return 1;
		}
		else
		{
			return 1e-10; //XXX 0;
		}
// 		if(VERBOSE > 5)
// 		{
// 			cerr<<"\t\talign "<<alignProb<<"\n";
// 		}

// 		if(next.word == _vocab.get("it", false))
// 		{
// 			return alignProb;
// 			//non-anaphoricity constant from EC's program
// //			return .1 * .147;
// 		}
// 		else
// 		{
// 			return 4*10e-4 * alignProb;
// 		}
	}

	if(contains(_nonAna, next.np->fullName()))
	{
		return 0;
	}

	if(constraintProhibits(next, prevWord))
	{
		return 0;
	}

	//number
	Prob pNum;
	if(person(Sent::head(next.tree)) == SECOND)
	{
		pNum = 1;
	}
	else
	{
		int anteNum = N_SING;

		stIndex htag = Sent::head(prevWord.tree)->label;
		if(htag == nnsLabel || htag == nnpsLabel)
		{
			anteNum = N_PLUR;
		}

		int proNum = plr(Sent::head(next.tree));

		pNum = _pSingGivenNum[anteNum];
//		cerr<<"detected "<<anteNum<<" "<<proNum<<" p sing "<<pNum<<"\n";
		if(proNum == N_PLUR)
		{
			pNum = 1 - pNum;
		}
	}

	//person
	int persAnte = person(Sent::head(prevWord.tree));
	int persPro = person(Sent::head(next.tree));

	int anteQtd = isQuoted(*prevWord.np);
	int proQtd = isQuoted(*prevWord.np);

	Prob pPers = _pPers[persAnte][persPro][2 * anteQtd + proQtd];

	//gender
	Prob pGen = 1;
	int gen = gender(Sent::head(next.tree));

	//life is actually more complicated here...
	// . is the gen entry for unknown word
	//for non-third pers loop over genders of prn

//	cerr<<"detected gen "<<gen<<" "<<prevWord.word<<"\n";
	if(gen != GEN_UNKNOWN)
	{
		int pos = _vocab.get(Sent::head(prevWord.tree)->label, false);
		assert(pos != -1);

		intIntProbMap& posMap = _pGen[pos];
		intIntProbMap::iterator en = posMap.find(prevWord.word);
		intProbMap* gens;
		if(en != posMap.end())
		{
			gens = &en->second;
		}
		else
		{
			//running the code suggests this is just turned off
			//gens = &posMap[_vocab.get(".", false)];
			gens = NULL;
		}

		if(gens)
		{
			if(persPro == THIRD)
			{
				pGen = (*gens)[gen];
			}
			else
			{
				intProbMap& proGens = _pGen[
					_vocab.get(Sent::head(next.tree)->label, false)]
					[next.word];
				pGen = 0;
				for(int gen = 0; gen < 3; ++gen)
				{
					pGen += (*gens)[gen] * proGens[gen];
				}
			}
		}
	}

	Prob res = alignProb * pNum * pPers * pGen;
	//note: .147 is returned by pGov when the governor mechanism
	//is turned off... why? I don't know
	if(VERBOSE > 5)
	{
		cerr<<"\t\talign "<<alignProb<<" num "<<pNum<<" pers "<<pPers<<
			" gen "<<pGen<<
			":: "<<res<<"\n";

	}
	assertProb(res);

	return res;
}

void PronounIBM::addPartialCounts(NounContext& next, ContextWord& prevWord,
								  MaxEntContext& alignContext, Prob count,
								  Prob total,
								  ContextWord* max)
{
	if(prevWord.word < _nulls)
	{
		//treat non-anaphoricity as observed... no counts added
		return;
	}

	if(contains(_nonAna, next.np->fullName()))
	{
		return;
	}

	if(count == 0)
	{
		return;
	}

	//XXX swbd only
// 	if(person(Sent::head(next.tree)) != THIRD)
// 	{
// 		return;
// 	}

	_alignment.addCount(prevWord.index, alignContext, count);
}

int PronounIBM::modType(Tree* next, Tree* prev)
{
	assert(next != NULL);
	Trees mods;
	collectMods(next, mods);

	if(mods.empty())
	{
		return NO_MODS;
	}
	else if(prev != NULL)
	{
		Trees prevMods;
		collectMods(prev, prevMods);
		foreach(Trees, prevMod, prevMods)
		{
			foreach(Trees, mod, mods)
			{
				if(Sent::identical(*prevMod, *mod))
				{
					return REPEAT_MODS;
				}
			}
		}
	}

	int mdStatus = HAS_MODS;
	foreach(Trees, mod, mods)
	{
		if(Sent::proper((*mod)->label))
		{
			if(mdStatus < PROPER_MODS)
			{
				mdStatus = PROPER_MODS;
			}
		}

		if(!Sent::preterm(*mod) && (*mod)->label != ppLabel &&
		   (*mod)->label != qpLabel)
		{
			if(mdStatus < PHRASAL_MODS)
			{
				mdStatus = PHRASAL_MODS;
			}
		}

		if((*mod)->label == cdLabel || (*mod)->label == qpLabel)
		{
			if(mdStatus < QUANT_MODS)
			{
				mdStatus = QUANT_MODS;
			}
		}

		if((*mod)->label == ppLabel)
		{
			if(Sent::headSym(*mod) == ofLabel)
			{
				if(mdStatus < PP_OF_MODS)
				{
					mdStatus = PP_OF_MODS;
				}
			}
			else
			{
				if(mdStatus < PP_MODS)
				{
					mdStatus = PP_MODS;
				}
			}
		}		
	}

	return mdStatus;
}

void PronounIBM::modTypes(Tree* next, Tree* prev, ints& res)
{
	assert(next != NULL);
	Trees mods;
	collectMods(next, mods);

	res.resize(REPEAT_MODS);
	for(int i = 0; i < REPEAT_MODS; ++i)
	{
		res[i] = 0;
	}

	if(mods.empty())
	{
		return;
	}

	Trees prevMods;
	if(prev != NULL)
	{
		collectMods(prev, prevMods);
	}

	foreach(Trees, mod, mods)
	{
		int mdStatus = HAS_MODS;

		if(Sent::proper((*mod)->label))
		{
			mdStatus = PROPER_MODS;
		}
		else if(!Sent::preterm(*mod) && (*mod)->label != ppLabel &&
				(*mod)->label != qpLabel)
		{
			mdStatus = PHRASAL_MODS;
		}
		else if((*mod)->label == cdLabel || (*mod)->label == qpLabel)
		{
			mdStatus = QUANT_MODS;
		}
		else if((*mod)->label == ppLabel)
		{
			if(Sent::headSym(*mod) == ofLabel)
			{
				mdStatus = PP_OF_MODS;
			}
			else
			{
				mdStatus = PP_MODS;
			}
		}

		bool foundIdent = false;
		foreach(Trees, prevMod, prevMods)
		{
			if(Sent::identical(*prevMod, *mod))
			{
				res[mdStatus] = 2;
				foundIdent = true;
				break;
			}
		}

		if(!foundIdent)
		{
			res[mdStatus] = 1;
		}
	}
}

void PronounIBM::collectMods(Tree* tree, Trees& mods)
{
	if(Sent::preterm(tree))
	{
		return;
	}

	for(Tree* i = tree->subtrees;
		i != NULL;
		i = i->sibling)
	{
		if(i->label != dtLabel && i->label != prpdLabel &&
		   i != Sent::headChild(tree) && 
		   i->label != posLabel && i->label != pdtLabel)
		{
			mods.push_back(i);
		}
		else if(i == Sent::headChild(tree))
		{
			collectMods(i, mods);
		}
	}
}

int PronounIBM::govType(Tree* tree)
{
	stIndex parInd = tree->parent->label;
	if(tree->parent->parent == NULL)
	{
		parInd = s1Label;
	}

	if(parInd == npLabel)
	{
		return 0;
	}
	if(parInd == vpLabel)
	{
		return 1;
	}
	if(parInd == ppLabel)
	{
		return 2;
	}
	if(Sent::sentType(tree->parent))
	{
		return 3;
	}
	return 4;
}

int PronounIBM::determinerType(Tree* tree)
{
	if(tree == NULL)
	{
		return NO_TREE;
	}

	if(tree->label != npLabel)
	{
		return NO_NP;
	}

	Tree* det = Sent::determiner(tree);
	int res = -1;

	if(!det)
	{
		//not an actual determiner, but an explicit possessor NP
		//syntactically fills the determiner slot, so we'll treat equivalently
		Tree* poss = Sent::head(tree)->parent->subtrees;
		if(Sent::hasChild(poss, posLabel))
		{
			return DT_POSSESSIVE;
		}
		res = NO_DET;
	}
	else if(det->label == prpdLabel)
	{
		res = DT_POSSESSIVE;
	}
	else if(Sent::word(det->subtrees) == "the")
	{
		res = DT_DEF;
	}
	else if(Sent::word(det->subtrees) == "a" || 
			Sent::word(det->subtrees) == "an")
	{
		res = DT_INDEF;
	}
	else if(Sent::word(det->subtrees) == "this" || 
			Sent::word(det->subtrees) == "that")
	{
		res = DT_DEICTIC;
	}
	else
	{
		res = DT_OTHER;
	}

	return res;
}

Prob PronounIBM::emTransferWordProb(int nextWord,
									NounContexts& global,
									NounContext& local, bool train,
									ContextWord*& max,
									IBM* otherModel)
{
	Prob res = ECIBM::emTransferWordProb(nextWord, global, local, train,
							  max, otherModel);

	assert(train);
	if(max->tree == NULL)
	{
		if(VERBOSE)
		{
			cerr<<"Marking as non-anaphoric: "<<local.np->fullName()<<"\n";
		}

		_nonAna.insert(local.np->fullName());
	}

	return res;
}

void PronounIBM::normalize()
{
	{
		intIntProbMap* temp = _transCounts;
		_transCounts = _newTransCounts;
		_newTransCounts = temp;
		_newTransCounts->clear();
	}

	{
		intProbMap* temp2 = _transTotals;
		_transTotals = _newTransTotals;
		_newTransTotals = temp2;
		_newTransTotals->clear();
	}

	if(Features::maxFeat() > _alignment.dimension())
	{
		cerr<<"setting dim "<<Features::maxFeat()<<"\n";
		_alignment.setDimension(Features::maxFeat());
	}
	_alignment.estimate();
	_alignment.clear();

// 	Prob* weights = _alignment.weights();
// 	for(int i = 0; i < _alignment.dimension(); ++i)
// 	{
// 		cerr<<i<<": "<<weights[i]<<"\n";
// 	}
}

void PronounIBM::distFeatsPostprocess(ContextWord& word, ints& feats)
{
// 	feats[F_WDIST] = feats[F_SENT - F_SYN];
// 	feats[F_SENT - F_SYN] = feats[F_SENT - F_SYN] * 6 + feats[F_WDIST - F_SYN];

	feats[F_LEX_ANIM - F_SYN] = -1;
	feats[F_LEX_FREQ - F_SYN] = -1;
	feats[F_ANC - F_SYN] = -1;
	feats[F_SAMECLAUSE - F_SYN] = -1;
}

string PronounIBM::modsToString(int mods)
{
	switch(mods)
	{
		case NO_MODS:
			return "none";
		case HAS_MODS:
			return "mods";
		case PP_MODS:
			return "pp";
		case PP_OF_MODS:
			return "pp-of";
		case QUANT_MODS:
			return "qp";
		case PHRASAL_MODS:
			return "phrase";
		case PROPER_MODS:
			return "proper";
		case REPEAT_MODS:
			return "repeat";
		default:
			std::ostringstream convert;
			convert<<"BUG("<<mods<<")";
			return string(convert.str());
//			abort();
	}
}

string PronounIBM::detToString(int det)
{
	switch(det)
	{
		case NO_TREE:
			return "0";
		case NO_NP:
			return "?";
		case NO_DET:
			return "-";
		case DT_POSSESSIVE:
			return "poss";
		case DT_DEF:
			return "def";
		case DT_INDEF:
			return "ind";
		case DT_DEICTIC:
			return "deic";
		case DT_OTHER:
			return "other";
		default:
			std::ostringstream convert;
			convert<<"BUG("<<det<<")";
			return string(convert.str());
//			abort();
	}
}

void PronounIBM::readLexicalStatistics(istream& is)
{
	strToProb wordToInanimate;
	readMap(wordToInanimate, is);
	Prob wordToInanimateTotal;
	is>>wordToInanimateTotal;

	strToProb wordToAnimate;
	readMap(wordToAnimate, is);
	Prob wordToAnimateTotal;
	is>>wordToAnimateTotal;

	Prob totalWords = wordToAnimateTotal + wordToInanimateTotal;

	foreach(strToProb, word, wordToInanimate)
	{
//		cerr<<"checking "<<word->first<<"\n";
		Prob countAnim = ifExists(wordToAnimate, word->first, 0.0);
		Prob totalCount = word->second + countAnim;

		if(totalCount >= 5)
		{
			int key = _vocab.get(lc(word->first), true);
			Prob pGivenAn = countAnim / wordToAnimateTotal;
			Prob pGivenIn = word->second / wordToInanimateTotal;
			Prob pAnim = pGivenAn / (pGivenAn + pGivenIn);
			_animacy[key] = pAnim;

			_frequency[key] = totalCount / totalWords;
		}
	}

	foreach(strToProb, word, wordToAnimate)
	{
		Prob countInanim = ifExists(wordToAnimate, word->first, 0.0);
		if(countInanim > 0)
		{
			continue;
		}

		if(word->second >= 5)
		{
			int key = _vocab.get(lc(word->first), true);
			Prob pAnim = 1;
			_animacy[key] = pAnim;

			_frequency[key] = word->second / totalWords;
		}
	}

// 	foreach(intToProb, word, _animacy)
// 	{
// 		cerr<<word->first<<": "<<
// 			_vocab.inv(word->first)<<"\t"<<word->second<<"\n";
// 	}
}
