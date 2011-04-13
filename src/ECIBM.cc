#include "ECIBM.h"
#include "treeInfo.h"
#include "setLabs.h"

ECIBM::ECIBM():
	TopicIBM(.01, 20)
{
	//it would be unwise to actually construct a model like this
}

ECIBM::ECIBM(istream& is):
	TopicIBM(.01, 20)
{
	read(is);
}

ECIBM::ECIBM(IBM& other):
	TopicIBM(other)
{
	ECIBM* oo = dynamic_cast<ECIBM*>(&other);
	if(oo)
	{
		_pSingGivenNum = oo->_pSingGivenNum;
		_pPers = oo->_pPers;
		_pGen = oo->_pGen;
		_pAnte = oo->_pAnte;
	}
}

void ECIBM::write(ostream& os)
{
	os<<"ECIBM\n";
	writeVector(_pAnte, os);
	writeHashMap(_pSingGivenNum, os);
	writeHashMap3(_pPers, os);
	writeHashMap3(_pGen, os);

	TopicIBM::write(os);
}

void ECIBM::read(istream& is)
{
	checkToken(is, "ECIBM");
	readVector(_pAnte, is);
	readHashMap(_pSingGivenNum, is);
	readHashMap3(_pPers, is);
	readHashMap3(_pGen, is);

	TopicIBM::read(is);

	//wtf is this for? this is really awful!
// 	for(int i = 0; i < _vocab.size(); ++i)
// 	{
// 		_produced.insert(i);
// 	}
}

void ECIBM::initCaches(Document& doc)
{
// 	INDEF.insert("all");
// 	INDEF.insert("anything");
// 	INDEF.insert("everybody");
// 	INDEF.insert("everything");
	INDEF.insert("one");
	INDEF.insert("ones");
	INDEF.insert("some");
	INDEF.insert("some");
	INDEF.insert("somebody");
// 	INDEF.insert("something");
//	INDEF.insert("stuff");
	INDEF.insert("that");
// 	INDEF.insert("thing");
// 	INDEF.insert("things");
	INDEF.insert("this");
	INDEF.insert("those");

	TopicIBM::initCaches(doc);

	bool inQ = false;
	foreach(Document, ss, doc)
	{
		Trees leaves;
		Sent::getLeaves((*ss)->tree(), leaves);

		foreach(Trees, leaf, leaves)
		{
			if(Sent::word((*leaf)->subtrees) == "``")
			{
				inQ = true;
			}
			else if(Sent::word((*leaf)->subtrees) == "''")
			{
				inQ = false;
			}

			if(inQ)
			{
				_quotedWords.insert(*leaf);
			}
		}
	}
}

void ECIBM::clearCaches()
{
	_quotedWords.clear();
}

void ECIBM::addNulls(NounContext& prev, string docName)
{
	for(int i = 0; i < _nulls; i++)
	{
		prev.push_back(ContextWord(i, NULL));
		ContextWord& word = prev.back();
//		string ti = "topic"+intToString(i);
//		word.feats[_alignmentFeatures.get(ti, true)] = 1;
//		word.feats.push_back(Feat(_alignmentFeatures.get(ti, true), 1));

		word.feats.push_back(Feat(Features::num(F_TOPIC, 0), 1));
	}
}

void ECIBM::getContextWords(Sent* sent, NounContext& res, bool add)
{
	foreach(NPs, np, sent->nps())
	{
		if((!isNoun(Sent::head((*np)->node())->label) &&
			!((*np)->pronoun())))
		{
			continue;
		}

		//note: ignores add
		int vind = _vocab.get((*np)->headSym(), true);
		if(vind == -1)
		{
			continue;
		}

		res.push_back(ContextWord(vind, (*np)->node()));
		ContextWord& created = res.back();
		created.np = *np;
		featurize(created, add);
	}
}

void ECIBM::getProduced(Sent* sent, NounContexts& res, bool add)
{
	foreach(NPs, np, sent->nps())
	{
		if(!(*np)->pronoun())
		{
			continue;
// 			if(!contains(INDEF, (*np)->head()))
// 			{
// 				continue;
// 			}
		}

		int vind = _vocab.get((*np)->headSym(), add);
		if(vind == -1)
		{
			continue;
		}

		int wInd = Sent::wordsBefore((*np)->node());
		res.push_back(NounContext(vind, (*np)->node()));
		NounContext& created = res.back();
		created.np = *np;

		foreach(NPs, otherNP, sent->nps())
		{
			if((!isNoun(Sent::head((*otherNP)->node())->label) &&
				!((*otherNP)->pronoun())))
			{
				continue;
			}

			//original constraint: no nesting
// 			int otherWInd = Sent::wordsBefore((*otherNP)->node());
// 			int otherLen = Sent::wordsDominated((*otherNP)->node());

// 			if(otherWInd + otherLen < wInd)

			int otherHWInd = Sent::wordsBefore(
				Sent::head((*otherNP)->node()));
			if(otherHWInd < wInd)
			{
//				cerr<<"add "<<*((*otherNP)->node())<<"\n";
				//note: ignores add
				int key = _vocab.get((*otherNP)->headSym(), true);
				if(key != -1)
				{
					created.push_back(ContextWord(key, (*otherNP)->node()));
					ContextWord& cw = created.back();
					cw.np = *otherNP;
					featurize(cw, add);
				}
			}
		}
	}
}

void ECIBM::featurize(ContextWord& created, bool add)
{
}

bool ECIBM::constraintProhibits(NounContext& next, ContextWord& prevWord)
{
	//first-pers outside quotes: anything
	if(person(Sent::head(next.tree)) == FIRST)
	{
		if(!isQuoted(*next.np))
		{
			return true;
		}
	}

	//2nd-person: anything
	if(person(Sent::head(next.tree)) == SECOND)
	{
		return true;
	}

	//referent is a non-anaphoric pronoun (can't be implemented in this system)

	//implemented by syntaxConstraint fn
	//preterm is EX: anything
	if(Sent::head(next.tree)->label == exLabel)
	{
		return true;
	}

	int ptype = proType(next.tree);
	bool reflexive = ptype == 1; //make enum!

	if(next.np->parent()->index() != prevWord.np->parent()->index())
	{
		//reflexive: outside this ss
		if(reflexive)
		{
			return true;
		}
		else
		{
			//all others outside this ss are ok
			return false;
		}
	}

	//pronoun is inside pp attached to ante: ante
	//not in EC: poss. prn is ok here (a man-i of his-i time)
	//but not: (*a man-i for him-i)
	if(ptype != 2)
	{
		Tree* par = next.tree->parent;
		if(par && par->label == ppLabel)
		{
			Tree* gpar = par->parent;

			if(gpar && Sent::head(gpar) == Sent::head(prevWord.tree))
			{
				return true;
			}
		}
	}

	//non-reflexive: ante is subj, pro is obj (simplified govt-and-binding)
	if(!reflexive && prevWord.np->role() == T_SUBJ && 
	   next.np->role() == T_OBJ)
	{
		return true;
	}

//refnum -2: appears to be used as blank init in scoring fn
//not clear what it means

	//ante can't be a prenominal without a real phrase
	if(isNoun(prevWord.np->node()->label))
	{
		return true;
	}

	return false;
}

Prob ECIBM::wordGivenPrevWord(NounContext& next, ContextWord& prevWord, 
							  bool train,
							  Prob totAlign, MaxEntContext& context)
{
	if(!next.np->pronoun())
	{
		return 1e-20;
	}

	if(prevWord.word < _nulls)
	{
		if(next.word == _vocab.get("it", false))
		{
			//non-anaphoricity constant from EC's program
			return .1 * .147;
		}
		else
		{
			return 4*10e-4;
		}
	}

	if(constraintProhibits(next, prevWord))
	{
		return 0;
	}

	//compute EC's terms, nevermind how they normalize

	//dist
	ints dummy;
	int atx = anteTabIndex(next, prevWord, dummy);
	assert(atx < _pAnte.size());
	Prob pAnte = _pAnte[atx];

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
		htag = Sent::head(next.tree)->label;
		//XXX for non-pronouns
		if(htag == nnsLabel || htag == nnpsLabel)
		{
			proNum = N_PLUR;
		}

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
		intProbMap* gens = getGenders(prevWord.tree);

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

	Prob res = pAnte * pNum * pPers * pGen * .147;
	//note: .147 is returned by pGov when the governor mechanism
	//is turned off... why? I don't know

	if(VERBOSE > 5)
	{
		cerr<<"\t\tante "<<pAnte<<" num "<<pNum<<" pers "<<pPers<<
			" gen "<<pGen<<
			":: "<<res<<"\n";
	}
	assertProb(res);

	return res;	
}

intProbMap* ECIBM::getGenders(Tree* tree)
{
	int pos = _vocab.get(Sent::head(tree)->label, false);
// 		if(pos == -1)
// 		{
// 			cerr<<*prevWord.np<<" "<<*prevWord.tree<<"\n";
// 		}
// 		assert(pos != -1);
	if(pos == -1)
	{
		return NULL;
	}
	else
	{
		intIntProbMap& posMap = _pGen[pos];
		int vind = _vocab.get(Sent::headSym(tree), false);
		intIntProbMap::iterator en = posMap.find(vind);
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
			//cerr<<"no gender for "<<Sent::headWord(tree)<<"\n";
		}
		return gens;
	}
}

int ECIBM::person(Tree* word)
{
	int proInd = -1;
	stIndex label = word->subtrees->label;
	for(int i=0;i<N_PRNS;i++)
	{
		if(label == prossi[i])
		{
			proInd = i;
			break;
		}
	}

	if(proInd == -1)
	{
		return THIRD;
	}
	if(proInd < 16)
	{
		return THIRD;
	}
	if(proInd < 20)
	{
		return FIRST;
	}
	if(proInd < 23)
	{
		return SECOND;
	}
	return FIRST;
}

int ECIBM::gender(Tree* word)
{
	int proInd = -1;
	stIndex label = word->subtrees->label;
	for(int i=0;i<N_PRNS;i++)
	{
		if(label == prossi[i])
		{
			proInd = i;
			break;
		}
	}

//	assert(proInd != -1);
	if(proInd == -1)
	{
		return GEN_UNKNOWN;
	}

	if(proInd <= 3)
	{
		return GEN_MASC;
	}
	if(proInd <= 7)
	{
		return GEN_FEM;
	}
	if(proInd <= 10)
	{
		return GEN_NEUT;
	}
	return GEN_UNKNOWN;
}

int ECIBM::proType(Tree* word)
{
	word = Sent::head(word);
	int proInd = -1;
	stIndex label = word->subtrees->label;
	for(int i=0;i<N_PRNS;i++)
	{
		if(label == prossi[i])
		{
			proInd = i;
			break;
		}
	}

	if(proInd == -1)
	{
		return 2 + anteType(word);
	}

	//poss.
	if(proInd==2)return 2;
	else if(proInd==6) return 2;
	else if(proInd ==9) return 2;
	else if(proInd==13)return 2;
	else if(proInd==14)return 2;
	else if(proInd==18)return 2;
	else if(proInd==21)return 2;
	else if(proInd==25)return 2;

	//reflx.
	if(proInd==3)return 1;
	else if(proInd==7)return 1;
	else if(proInd==10)return 1;
	else if(proInd==15)return 1;
	else if(proInd==19)return 1;
	else if(proInd==22)return 1;
	else if(proInd==26)return 1;

	else return 0;
}

int ECIBM::anteType(Tree* word)
{
	if(isPron(Sent::head(word)))
	{
		return 0;
	}

	stIndex htag = Sent::head(word)->label;
	if(Sent::proper(htag))
	{
		return 2;
	}
	else if(isNoun(htag))
	{
		return 3;
	}
	else return 1;
}

bool ECIBM::isQuoted(NP& np)
{
	return contains(_quotedWords, Sent::head(np.node()));
}

int ECIBM::bucketDist(int dist)
{
  int bucks[5] = {2,4,8,16,32};
  int ans=0;
  for(;ans<5;ans++)
    if(dist<=bucks[ans])return ans;
  return ans;
}

int ECIBM::bucketSent(int dist)
{
	int bucks[5] = {0,1,2,4,8};
	int ans=0;
	for(;ans<5;ans++)
	{
		if(dist<=bucks[ans])
		{
			return ans;
		}
	}
	return ans;
}

int ECIBM::anteTabIndex(NounContext& next, ContextWord& prevWord, ints& feats)
{
	int syn;
	switch(prevWord.np->role())
	{
		case T_SUBJ:
			syn = 0;
			break;
		case T_OBJ:
			syn = 1;
			break;
		case T_X:
			syn = 2;
			break;
		default:
			cerr<<"Can't convert.\n";
			assert(0);
	}

	//num sents between pro and ante
	int sent = next.np->parent()->index() - prevWord.np->parent()->index();
	if(sent >= _prevSents)
	{
		//happens if we skip a sent with no context items
		sent = _prevSents;
	}
	sent = bucketSent(sent);

	//n words before prev
	int wdist = Sent::wordsBefore(Sent::head(prevWord.tree));
	int proWordPos = Sent::wordsBefore(Sent::head(next.tree));
	if(sent == 0)
	{
		//or dist between the two, if same stt
		wdist = proWordPos - prevWord.wordInd;
	}
	wdist = bucketDist(wdist);

// 	cerr<<*(prevWord.np->node())<<" "<<*(next.np->node())<<"\n";
// 	cerr<<" pr s "<<prevWord.np->parent()->index()<<" "<<next.np->parent()->index()<<"\n";
// 	cerr<<sent<<" "<<wdist<<" "<<proWordPos<<"\n";

	//boring math to combine all
	int cmb = sent * ANTESENTSEP;
	cmb += syn * SYNTAXNOSEP;
	cmb += wdist * ANTEWORDSEP;

	//pronoun word pos in sentence
	proWordPos = bucketDist(proWordPos);
	if(proWordPos > 3)
	{
		proWordPos = 3;
	}
	cmb += proWordPos * PROTYPSEP;

	//term for type of prn
	int proT = proType(next.tree);
	cmb += ANTETYPSEP * proT;

	//term for type of ante
	int anteT = anteType(prevWord.tree);
	cmb += anteT;

// 	cerr<<"syn "<<syn<<" sd "<<sent<<" wd "<<wdist<<" d2 "<<
// 		proWordPos<<" at "<<anteType(prevWord.tree)<<" pt "<<
// 		proType(Sent::head(next.tree))<<" cmb "<<cmb<<"\n";

//	assert(cmb < ANTEDISTSZ); //may not be true in derived classes
	assert(cmb >= 0);

	feats.push_back(syn);
	feats.push_back(sent);
	feats.push_back(wdist);
	feats.push_back(proWordPos);
	feats.push_back(proT);
	feats.push_back(anteT);

	return cmb;
}
//EC's actual code:
//   int synNo=ai.ante->syntaxNo;
//   if(synNo>=PASSOBJ&&synNo<LASTSYN)synNo=SUBJECT;

//   if(synNo==IS)synNo=SUBJECT;
//   else if(synNo>3)synNo=2;
//   assert(synNo<3);
//   int sdist = stI.sentNo-ai.ante->sentNo;
//   if(sdist>=3){
//     cerr<<sdist<<endl;
//     assert(sdist<3);
//   }
//   int dist = ai.ante->wordNo;
//   if(sdist==0)dist=stI.wordNo-dist;
//   int cmb =sdist*ANTESENTSEP;
//   if(stage>=ANTE2){
//     cmb += synNo*SYNTAXNOSEP;
//     cmb += bucketDist(dist)*ANTEWORDSEP;
//     int d2=bucketDist(stI.wordNo);
//     if(d2>3)d2=3;
//     cmb+=d2*PROTYPSEP;
//   }
//   if(stage>=ANTE3){
//     cmb+= ANTETYPSEP*proType(ai.pro->wrd);
//     int at=anteType(ai.ante->pretrm,ai.ante->wrd,stI);
//     cmb+=at;
//   }
  
//   double prb=ai.prob;
//   assert(cmb<ANTEDISTSZ);
//   DdTrip& ddp=pAntegDist[cmb];
//   //double ans=ddp.prob;
//   double ans=distributeProb(ddp.prob,cmb);
//   //cerr<<"\t"<<ans<<" "<<ddp.prob<<" "<<cmb<<endl;
//   if(ai.prob>=0){
//     pthread_mutex_lock(&pagdlock);
//     ddp.pos+=ai.prob;
//     ddp.cnt++;
//     pthread_mutex_unlock(&pagdlock);
//   }
//   return ans;

void ECIBM::addPartialCounts(NounContext& nextWord, ContextWord& prevWord,
							 MaxEntContext& alignContext, Prob count,
							 Prob total,
							 ContextWord* max)
{
	cerr<<"Warning: don't do this.\n";
	assert(0);
}
