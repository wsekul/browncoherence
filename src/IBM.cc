#include "IBM.h"

#include "treeInfo.h"
#include "digamma.h"

IBM::IBM(double smooth, int nulls):
	_prevSents(0),
	_estimating(false),
	_smooth(smooth),
	_nulls(nulls),
	_alignment(0, RNG),
	VERBOSE(0)
{
	_alignmentFeatures.get("BIAS", true);

	_transCounts = &_counts1;
	_newTransCounts = &_counts2;

	_transTotals = &_totals1;
	_newTransTotals = &_totals2;

	int minNulls = 20;
	if(_nulls > minNulls)
	{
		minNulls = _nulls;
	}

	//reserve space to use for up to 20 null words, even if we don't
	//need them right now
	for(int null = 0; null < minNulls; ++null)
	{
		_vocab.get("null"+intToString(null), true);
	}
}

IBM::IBM(istream& in):
	_alignment(0, RNG)
{
	read(in);
}

IBM::IBM(IBM& other):
	_prevSents(0),
	_smooth(other._smooth),
	_nulls(other._nulls),
	_counts1(other._counts1),
	_counts2(other._counts2),
	_totals1(other._totals1),
	_totals2(other._totals2),
	_vocab(other._vocab),
	_alignmentFeatures(other._alignmentFeatures),
	_alignment(other._alignment)
{
	_estimating = true;
	VERBOSE = 0;

	if(other._transCounts == &other._counts1)
	{
		_transCounts = &_counts1;
		_newTransCounts = &_counts2;
	}
	else
	{
		assert(other._transCounts == &other._counts2);
		_transCounts = &_counts2;
		_newTransCounts = &_counts1;
	}

	if(other._transTotals == &other._totals1)
	{
		_transTotals = &_totals1;
		_newTransTotals = &_totals2;
	}
	else
	{
		assert(other._transTotals == &other._totals2);
		_transTotals = &_totals2;
		_newTransTotals = &_totals1;
	}
}

IBM::~IBM()
{
}

void IBM::initCaches(Document& doc)
{
}

void IBM::clearCaches()
{
}

int IBM::history()
{
//	return 1;
	return _prevSents + 2;
}

//two serialization methods: should be inverses of each other

//serializes this class
void IBM::write(ostream& os)
{
	os<<"IBM_MODEL\n";
 	os<<_prevSents<<"\t"<<_smooth<<"\t"<<_nulls<<"\n";

 	writeSet(_produced, os);
 	writeHashMap(*_transTotals, os);
 	writeHashMap2(*_transCounts, os);
 	writeHashMap(_addedFactor, os);

 	_vocab.write(os);
	_alignmentFeatures.write(os);

	_alignment.write(os);
}

//initializes this class -- called by the istream constructor
void IBM::read(istream& is)
{
	checkToken(is, "IBM_MODEL");

 	is>>_prevSents>>_smooth>>_nulls;

 	readSet(_produced, is);
 	readHashMap(_totals1, is);
 	readHashMap2(_counts1, is);
 	readHashMap(_addedFactor, is);

	_vocab.read(is);
	_alignmentFeatures.read(is);
	_alignment.read(is);

 	_transCounts = &_counts1;
	_newTransCounts = &_counts2;
	_transTotals = &_totals1;
	_newTransTotals = &_totals2;

	if(VERBOSE)
	{
		_vocab.invert();
	}
}

void IBM::writeCounts(ostream& os)
{
	_vocab.write(os);
	_alignmentFeatures.write(os);
	writeSet(_produced, os);
	writeHashMap(*_newTransTotals, os);
	writeHashMap2(*_newTransCounts, os);
}

void IBM::readCounts(istream& is, intIntMap& vocabTrans)
{
	string word;
	int theirVocab;

	while(is>>word)
	{
		if(word == ">>")
		{
			break;
		}
		is>>theirVocab;

		int ourVocab = _vocab.get(word, true);
		vocabTrans[theirVocab] = ourVocab;
	}

	while(is>>word)
	{
		if(word == ">>")
		{
			break;
		}
		is>>theirVocab;

		_alignmentFeatures.get(word, true);
	}

	//easier but inefficient update
	intSet produced;
	readSet(produced, is);
	for(intSet::iterator i = produced.begin();
		i != produced.end();
		i++)
	{
		_produced.insert(vocabTrans[*i]);
	}

	//totals
	while(is>>word)
	{
		if(word == ">>")
		{
			break;
		}
		int ourSym = vocabTrans[atoi(word.c_str())];
		Prob count;
		is>>count;
		(*_newTransTotals)[ourSym] += count;
	}

	//counts
	while(is>>word)
	{
		if(word == ">>>")
		{
			break;
		}
		//semi-efficient
		int ourSym = vocabTrans[atoi(word.c_str())];
		intProbMap& ourTab = (*_newTransCounts)[ourSym];

		intToProb subtab;
		readMap(subtab, is);
		for(intToProb::iterator i = subtab.begin();
			i != subtab.end();
			i++)
		{
			ourTab[vocabTrans[i->first]] += i->second;
		}
	}
}

void IBM::print(ostream& os)
{
	_vocab.invert();

	const Prob thresh = 1e-20;

	for(intIntProbMap::iterator from = _transCounts->begin();
		from != _transCounts->end();
		from++)
	{
		PriVec sorter;
		for(intProbMap::iterator to = from->second.begin();
			to != from->second.end();
			to++)
		{
			NounContext nc(to->first, NULL);
			ContextWord cc(from->first, NULL);
			sorter.push_back(PriNode(transProb(nc, cc),
									 _vocab.inv(to->first)));
		}
		sort(sorter.begin(), sorter.end(), Greater());
		if(sorter.size() >= 2 && sorter[1].pri > thresh)
		{
			os<<_vocab.inv(from->first)<<"\n";

			//note: 0 cannot be produced, and thus the
			//call below measures the addedFactor term
			NounContext nc(0, NULL);
			ContextWord cc(from->first, NULL);

			os<<"\t"<<"OTHERS"<<"\t"<<transProb(nc, cc)<<"\n";


			int toPrint = 400;
			int nPoss = sorter.size();
			for(int pr = 0; pr < nPoss && pr != toPrint; ++pr)
			{
				if(sorter[pr].pri > thresh)
				{
					os<<"\t"<<sorter[pr].data<<"\t"<<sorter[pr].pri<<"\n";
				}
			}

			os<<"\n";
		}
	}

	os<<"Alignment:\n";

	_alignmentFeatures.invert();
	int dim = _alignment.dimension();
	Prob* weights = _alignment.weights();
	for(int wt = 0; wt < dim; ++wt)
	{
		string fname = _alignmentFeatures.inv(wt);
		if(fname.substr(0, 15) == "../switchboard/")
		{
			continue;
		}
		os<<fname<<":\t"<<weights[wt]<<"\n";
	}
}

void IBM::projectOntoTopics(Document& doc, Probs& res)
{
	NounContext nulls;
	addNulls(nulls, doc.name());

	MaxEntContext allNull;
	nulls.addToContext(allNull);

	for(int nn = 0; nn < _nulls; ++nn)
	{
		res.push_back(_alignment.probToken(nn, allNull));
	}
}

void IBM::nullContext(NounContext*& nulls, MaxEntContext*& allNull)
{
	nulls = new NounContext();
	addNulls(*nulls, string("foo"));
	allNull = new MaxEntContext();
	nulls->addToContext(*allNull);
}

Prob IBM::projectWord(int word, NounContext* nulls, MaxEntContext* allNull, 
					  Probs& projection)
{
	nulls->word = word;

	Prob norm = 0;

	for(ContextWords::iterator prevWord = nulls->begin();
		prevWord != nulls->end();
		prevWord++)
	{
		Prob pGiven = wordGivenPrevWord(*nulls, *prevWord,
										false, 0, 
										*allNull);
		projection[prevWord->word] = pGiven;
		norm += pGiven;
	}

	return norm;
}

void IBM::estimate()
{
	cerr<<"Can't do one-pass estimation... use TrainIBM.\n";
}

void IBM::setEstimating(bool status)
{
	_estimating = status;
}

Prob IBM::review(Document& doc)
{
	initCaches(doc);

	ints perm;
	for(int i=0; i<doc.size(); i++)
	{
		perm.push_back(i);
	}

	Prob res = permProbability(doc, perm, true);

	clearCaches();

	return res;
}

ProdToContext& IBM::maxAlignments()
{
	return _maxAlignments;
}

//log probability of a permuted document
//or does training
Prob IBM::permProbability(Document& doc, ints& perm, bool train)
{
	_sentScores.clear();
	_sentScores.resize(perm.size());
	_globalScore = 0;

	double docProb = 0.0;

	//note: the prev structure holds a bunch of NounContext, 1 for each
	//sentence
	//cell 0 is reserved for NULL words
	//cell i in [1..n] holds context for the sentence i before this one
	NounContexts prev;
	prev.push_back(NounContext()); //push cell 0

	for(int i = 1; i <= _prevSents; i++)
	{
		prev.push_back(NounContext()); //push a cell for each prev st
		prev[i].sent = i;
	}

	NounContext next;
	NounContexts produced;
	addNulls(prev[0], doc.name());

	int sentCtr = 0;
	for(ints::iterator p = perm.begin();
		p != perm.end();
		p++,sentCtr++)
	{
		Sent* sent = doc[*p];
		Prob sentProb = 0.0;

		//words will be added to vocab if training mode is on
		//fill in context vector with all objects to condition on for
		//sentence *after* this one
		//fill produced vector with all objects to actually produce
		getContextWords(sent, next, train);
		getProduced(sent, produced, train);

		if(VERBOSE > 14)
		{
			foreach(NounContext, cw, next)
			{
				cerr<<"Context: "<<_vocab.inv(cw->word)<<"\n";
			}
		}

		if(VERBOSE)
		{
			cerr<<*sent<<"\n";
		}

		if(VERBOSE > 1)
		{
			cerr<<"Producing "<<produced.size()<<" words.\n";
			_vocab.invert();
		}

		//have to generate each one of these words somehow
		for(NounContexts::iterator prod = produced.begin();
			prod != produced.end();
			prod++)
		{
			int nextWord = prod->word;
			if(VERBOSE > 1)
			{
				cerr<<"Producing "<<_vocab.inv(nextWord)<<"\n";
			}

			ContextWord* max = NULL;
			Prob wordProb = wordProbability(nextWord, prev, *prod, train,
				max);
			assert(max != NULL);
			_maxAlignments[*prod] = *max;

			if(VERBOSE > 1)
			{
				cerr<<"\t "<<_vocab.inv(nextWord)<<" prob "<<wordProb<<"\n";
			}

			sentProb += log(wordProb);
		}

		_sentScores[sentCtr] = sentProb;
		docProb += sentProb;
		//stupid floating-pt tolerance!
		if(fabs(docProb) < 1e-5 && docProb > 0)
		{
			docProb = 0;
		}
		assertLogProb(docProb);

		if(!next.empty()) //if sentence is bogus, don't update context
		{
			//kick out oldest context element (not 0, which holds the NULLs)
			for(int i = _prevSents; i > 1; i--)
			{
				prev[i] = prev[i-1];
				prev[i].sent = i;
			}
			//and add the newest context vector
			if(_prevSents >= 1)
			{
				prev[1] = next;
				prev[1].sent = 1;
			}
		}
		else
		{
			if(VERBOSE > 5)
			{
				cerr<<"Bogus sentence, skipping.\n";
			}
		}

		next.clear();
		produced.clear();

		if(VERBOSE)
		{
			cerr<<"\n";
		}
	}

	//!! returns a probability, even in training mode--
	// just don't turn on training by mistake
	return docProb;
}

Prob IBM::emTransfer(Document& doc, ints& perm, bool train, IBM* otherModel)
{
	double docProb = 0.0;

	//note: the prev structure holds a bunch of NounContext, 1 for each
	//sentence
	//cell 0 is reserved for NULL words
	//cell i in [1..n] holds context for the sentence i before this one
	NounContexts prev;
	prev.push_back(NounContext()); //push cell 0

	for(int i = 1; i <= _prevSents; i++)
	{
		prev.push_back(NounContext()); //push a cell for each prev st
		prev[i].sent = i;
	}

	NounContext next;
	NounContexts produced;
	addNulls(prev[0], doc.name());

	for(ints::iterator p = perm.begin();
		p != perm.end();
		p++)
	{
		Sent* sent = doc[*p];
		Prob sentProb = 0.0;

		//words will be added to vocab if training mode is on
		//fill in context vector with all objects to condition on for
		//sentence *after* this one
		//fill produced vector with all objects to actually produce
		getContextWords(sent, next, train);
		getProduced(sent, produced, train);

		if(VERBOSE > 14)
		{
			foreach(NounContext, cw, next)
			{
				cerr<<"Context: "<<_vocab.inv(cw->word)<<"\n";
			}
		}

		if(VERBOSE)
		{
			cerr<<*sent<<"\n";
		}

		if(VERBOSE > 1)
		{
			cerr<<"Producing "<<produced.size()<<" words.\n";
			_vocab.invert();
		}

		//have to generate each one of these words somehow
		for(NounContexts::iterator prod = produced.begin();
			prod != produced.end();
			prod++)
		{
			int nextWord = prod->word;
			if(VERBOSE > 1)
			{
				cerr<<"Producing "<<_vocab.inv(nextWord)<<"\n";
			}

			ContextWord* max = NULL;
			Prob wordProb = emTransferWordProb(
				nextWord, prev, *prod, train, max, otherModel);
			assert(max != NULL);
			_maxAlignments[*prod] = *max;

			if(VERBOSE > 1)
			{
				cerr<<"\t "<<_vocab.inv(nextWord)<<" prob "<<wordProb<<"\n";
			}

			sentProb += log(wordProb);
		}

		docProb += sentProb;
		assertLogProb(docProb);

		if(!next.empty()) //if sentence is bogus, don't update context
		{
			//kick out oldest context element (not 0, which holds the NULLs)
			for(int i = _prevSents; i > 1; i--)
			{
				prev[i] = prev[i-1];
				prev[i].sent = i;
			}
			//and add the newest context vector
			if(_prevSents >= 1)
			{
				prev[1] = next;
				prev[1].sent = 1;
			}
		}
		else
		{
			if(VERBOSE > 5)
			{
				cerr<<"Bogus sentence, skipping.\n";
			}
		}

		next.clear();
		produced.clear();

		if(VERBOSE)
		{
			cerr<<"\n";
		}
	}

	//!! returns a probability, even in training mode--
	// just don't turn on training by mistake
	return docProb;
}

void IBM::addNulls(NounContext& prev, string docName)
{
	for(int i = 0; i < _nulls; i++)
	{
		prev.push_back(ContextWord(i, NULL));
		ContextWord& word = prev.back();
		string ti = "topic"+intToString(i);
//		word.feats[_alignmentFeatures.get(ti, true)] = 1;
		word.feats.push_back(
			Feat(_alignmentFeatures.get(ti, true), 1));
		string docTi = docName + ti;
//		word.feats[_alignmentFeatures.get(docTi, true)] = 1;
		int fnum = _alignmentFeatures.get(docTi, false);
		if(fnum != -1)
		{
			word.feats.push_back(Feat(fnum, 1));
		}
	}
}

Prob IBM::wordProbability(int nextWord,
						  NounContexts& global,
						  NounContext& local, bool train)
{
	ContextWord* dummy = NULL;
	return wordProbability(nextWord, global, local, train, dummy);
}

Prob IBM::wordProbability(int nextWord,
						  NounContexts& global,
						  NounContext& local, bool train,
						  ContextWord*& max)
{
	if(train && !_estimating)
	{
		_produced.insert(nextWord);
	}
	else if(train && !contains(_produced, nextWord))
	{
		if(VERBOSE)
		{
			cerr<<"\t"<<_vocab.inv(nextWord)<<
				" is in vocab but not produced.\n";
		}
		return 1.0;
	}

	Prob wordProb = 0.0;

	Prob totAlign = 0; //used for initial run
	MaxEntContext alignContext; //used for normal running

	featurizeDistances(local, global, train && !_estimating);

	//calculate the set of items which we select alignments from
	for(NounContexts::iterator context = global.begin();
		context != global.end();
		context++)
	{
		context->addToContext(alignContext);

		totAlign += context->size();
	}
	local.addToContext(alignContext);

	totAlign += local.size();

	if(VERBOSE > 15)
	{
		MaxEntSelection::printContext(alignContext, cerr);
		cerr<<"\n";
	}

	Prob bestProb = 0;

	Prob probs[alignContext.size()];
	int probInd = 0;
	for(NounContexts::iterator context = global.begin();
		context != global.end();
		context++)
	{
		for(ContextWords::iterator prevWord = context->begin();
			prevWord != context->end();
			prevWord++, probInd++)
		{
			Prob pGiven = wordGivenPrevWord(local, *prevWord,
											train, totAlign, 
											alignContext);

			if(VERBOSE > 3)
			{
				cerr<<"\tProb given "<<_vocab.inv(prevWord->word)<<
					" "<<pGiven<<"\n";
			}

			probs[probInd] = pGiven;
			wordProb += pGiven;

			if(VERBOSE > 15)
			{
				cerr<<"Normalization now "<<wordProb<<"\n";
			}		

			if(pGiven > bestProb)
			{
				bestProb = pGiven;
				max = &*prevWord;
			}
		}
	}
	for(ContextWords::iterator prevWord = local.begin();
		prevWord != local.end();
		prevWord++, probInd++)
	{
		Prob pGiven = wordGivenPrevWord(local, *prevWord,
										train, totAlign, 
										alignContext);
		
		if(VERBOSE > 3)
		{
			cerr<<"\tProb given local "<<_vocab.inv(prevWord->word)<<
				" "<<pGiven<<"\n";
		}

		probs[probInd] = pGiven;
		wordProb += pGiven;

		if(VERBOSE > 15)
		{
			cerr<<"Normalization now "<<wordProb<<"\n";
		}

		if(pGiven > bestProb)
		{
			bestProb = pGiven;
			max = &*prevWord;
		}
	}

	if(train)
	{
		probInd = 0;
		for(NounContexts::iterator context = global.begin();
			context != global.end();
			context++)
		{
			for(ContextWords::iterator prevWord = context->begin();
				prevWord != context->end();
				prevWord++, probInd++)
			{
// 				Prob pGiven = wordGivenPrevWord(
// 					local, *prevWord, train, totAlign, alignContext);
				Prob pGiven = probs[probInd];

				if(VERBOSE > 14)
				{
					cerr<<"\tNormalized prob "<<_vocab.inv(prevWord->word)<<
						" "<<pGiven<<"\t";
				}

				pGiven /= wordProb;

				if(VERBOSE > 14)
				{
					cerr<<pGiven<<" "<<wordProb<<"\n";
				}

				addPartialCounts(local, *prevWord, alignContext, 
								 pGiven, wordProb, max);
			}
		}
		for(ContextWords::iterator prevWord = local.begin();
			prevWord != local.end();
			prevWord++, probInd++)
		{
// 			Prob pGiven = wordGivenPrevWord(
// 				local, *prevWord, train, totAlign, alignContext);
			Prob pGiven = probs[probInd];

			if(VERBOSE > 14)
			{
				cerr<<"\tNormalized prob "<<_vocab.inv(prevWord->word)<<
					" "<<pGiven<<"\t";
			}

			pGiven /= wordProb;

			if(VERBOSE > 14)
			{
				cerr<<pGiven<<" "<<wordProb<<"\n";
			}

			addPartialCounts(local, *prevWord, alignContext, 
							 pGiven, wordProb, max);
		}
	}

	assertProb(wordProb);
	return wordProb;
}

Prob IBM::emTransferWordProb(int nextWord,
							 NounContexts& global,
							 NounContext& local, bool train,
							 ContextWord*& max,
							 IBM* otherModel)
{
	if(train && !_estimating)
	{
		_produced.insert(nextWord);
	}
	else if(!contains(_produced, nextWord))
	{
		if(VERBOSE)
		{
			cerr<<"\t"<<_vocab.inv(nextWord)<<
				" is in vocab but not produced.\n";
		}
		return 1.0;
	}

	Prob wordProb = 0.0;

	Prob totAlign = 0; //used for initial run
	MaxEntContext alignContext; //used for normal running

	featurizeDistances(local, global, train && !_estimating);

	//calculate the set of items which we select alignments from
	for(NounContexts::iterator context = global.begin();
		context != global.end();
		context++)
	{
		context->addToContext(alignContext);

		totAlign += context->size();
	}
	local.addToContext(alignContext);

	totAlign += local.size();

	if(VERBOSE > 15)
	{
		MaxEntSelection::printContext(alignContext, cerr);
		cerr<<"\n";
	}

	Prob bestProb = 0;

	for(NounContexts::iterator context = global.begin();
		context != global.end();
		context++)
	{
		for(ContextWords::iterator prevWord = context->begin();
			prevWord != context->end();
			prevWord++)
		{
			Prob pGiven = otherModel->wordGivenPrevWord(local, *prevWord,
											train, totAlign, 
											alignContext);

			if(VERBOSE > 3)
			{
				cerr<<"\tProb given "<<_vocab.inv(prevWord->word)<<
					" "<<pGiven<<"\n";
			}

			wordProb += pGiven;

			if(VERBOSE > 15)
			{
				cerr<<"Normalization now "<<wordProb<<"\n";
			}		

			if(pGiven > bestProb)
			{
				bestProb = pGiven;
				max = &*prevWord;
			}
		}
	}
	for(ContextWords::iterator prevWord = local.begin();
		prevWord != local.end();
		prevWord++)
	{
		Prob pGiven = otherModel->wordGivenPrevWord(local, *prevWord,
										train, totAlign, 
										alignContext);
		
		if(VERBOSE > 3)
		{
			cerr<<"\tProb given local "<<_vocab.inv(prevWord->word)<<
				" "<<pGiven<<"\n";
		}

		wordProb += pGiven;

		if(VERBOSE > 15)
		{
			cerr<<"Normalization now "<<wordProb<<"\n";
		}

		if(pGiven > bestProb)
		{
			bestProb = pGiven;
			max = &*prevWord;
		}
	}

	if(train)
	{
		for(NounContexts::iterator context = global.begin();
			context != global.end();
			context++)
		{
			for(ContextWords::iterator prevWord = context->begin();
				prevWord != context->end();
				prevWord++)
			{
				Prob pGiven = otherModel->wordGivenPrevWord(
					local, *prevWord, train, totAlign, alignContext);

				if(VERBOSE > 14)
				{
					cerr<<"\tNormalized prob "<<_vocab.inv(prevWord->word)<<
						" "<<pGiven<<"\t";
				}

				pGiven /= wordProb;

				if(VERBOSE > 14)
				{
					cerr<<pGiven<<" "<<wordProb<<"\n";
				}

				addPartialCounts(local, *prevWord, alignContext, 
								 pGiven, wordProb, max);
			}
		}
		for(ContextWords::iterator prevWord = local.begin();
			prevWord != local.end();
			prevWord++)
		{
			Prob pGiven = otherModel->wordGivenPrevWord(
				local, *prevWord, train, totAlign, alignContext);

			if(VERBOSE > 14)
			{
				cerr<<"\tNormalized prob "<<_vocab.inv(prevWord->word)<<
					" "<<pGiven<<"\t";
			}

			pGiven /= wordProb;

			if(VERBOSE > 14)
			{
				cerr<<pGiven<<" "<<wordProb<<"\n";
			}

			addPartialCounts(local, *prevWord, alignContext,
							 pGiven, wordProb, max);
		}
	}

	assertProb(wordProb);
	return wordProb;
}

void IBM::addPartialCounts(NounContext& next,
						   ContextWord& prevWord, 
						   MaxEntContext& alignContext,
						   Prob pGiven,
						   Prob wordProb,
						   ContextWord* max)
{
	if(pGiven == 0)
	{
		return;
	}

	const int nextWord = next.word;
	(*_newTransCounts)[prevWord.word][nextWord] += pGiven;
	(*_newTransTotals)[prevWord.word] += pGiven;
	assertProb(pGiven);
	_alignment.addCount(prevWord.index, alignContext, pGiven);
}

void IBM::featurizeDistances(NounContext& local, NounContexts& global,
	bool add)
{
	int wordInd = Sent::wordsBefore(local.tree);
	foreach(NounContexts, context, global)
	{
		int sent = context->sent;
		foreach(NounContext, word, *context)
		{
			int sourceInd = word->wordInd;
			if(sourceInd != -1) //dist feats not set for nulls
			{
				int wdist = sourceInd;
				word->clearDistFeats();

				string sentDist = "sent" + intToString(bucket(sent));
				string fullDistFeat = sentDist +
					"-" + intToString(bucket(wdist));
				int fname = _alignmentFeatures.get(fullDistFeat, add);
				if(fname != -1)
				{
					word->addDistFeat(fname);
				}
				fname = _alignmentFeatures.get(sentDist, add);
				if(fname != -1)
				{
					word->addDistFeat(fname);
				}
			}
		}
	}

	foreach(NounContext, word, local)
	{
		int sourceInd = word->wordInd;
		if(sourceInd != -1) //dist feats not set for nulls
		{
			int wdist = wordInd - sourceInd;

			word->clearDistFeats();

			int sent = 0;
// 			if(word->overrideSent != -1)
// 			{
// 				sent = word->overrideSent;
// 			}

			string sentDist = "sent" + intToString(bucket(sent));
			string fullDistFeat = sentDist +
				"-" + intToString(bucket(wdist));
			int fname = _alignmentFeatures.get(fullDistFeat, add);
			if(fname != -1)
			{
				word->addDistFeat(fname);
			}
			fname = _alignmentFeatures.get(sentDist, add);
			if(fname != -1)
			{
				word->addDistFeat(fname);
			}
		}
	}
}

int IBM::bucket(int d)
{
	if(d == 0)
	{
		return 0;
	}
	if(d < 0)
	{
		return -1;
	}

	int bucket = (int)(log(d)/log(2));
	if(bucket > 4)
	{
		bucket = 4;
	}
	return bucket;
}

Prob IBM::getRandBias(int null, int word)
{
	return getRandBiasHelper(null, word, *_transCounts, *_transTotals);
}

Prob IBM::getRandBiasHelper(int null, int word, intIntProbMap& transCounts,
							intProbMap& transTotals)
{
	intProbMap& bTab = (transCounts)[null];
	intProbMap::iterator entry = bTab.find(word);
	if(entry != bTab.end())
	{
		return entry->second;
	}
	Prob key = 10 * gsl_rng_uniform_pos(RNG);
	bTab[word] = key;
	(transTotals)[null] += key;

	return key;
}

void IBM::normalize()
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

	cerr<<"setting dim "<<_alignmentFeatures.size()<<"\n";
	_alignment.setDimension(_alignmentFeatures.size());
	_alignment.estimate();
	_alignment.clear();

// 	Prob* weights = _alignment.weights();
// 	for(int i = 0; i < _alignment.dimension(); ++i)
// 	{
// 		cerr<<i<<": "<<weights[i]<<"\n";
// 	}
}

void IBM::smooth()
{
	//assume addedFactor now reflects last iteration's smoothing
	//addedFactor will hold the probability for items with no real count
	_addedFactor.clear();

	for(intIntProbMap::iterator i = _newTransCounts->begin();
		i != _newTransCounts->end();
		i++)
	{
		if(i->first < _nulls)
		{
			smoothVB(i->first, i->second, _newTransTotals,
					 _addedFactor, 1.0);
		}
		else
		{
			smoothVB(i->first, i->second, _newTransTotals,
					 _addedFactor, _smooth);
		}
	}
}

void IBM::smoothVB(int word, intProbMap& counts, intProbMap* totals,
				   intProbMap& addedFactor,
				   Prob prior)
{
	intProbMap::iterator total = totals->find(word);
	assert(total != totals->end());

	//words with counts
	Prob maxCt = 0;
	for(intProbMap::iterator j = counts.begin();
		j != counts.end();
		j++)
	{
		j->second += prior;

		j->second = exp(digamma(j->second));
		if(j->second > maxCt)
		{
			maxCt = j->second;
		}

		if(j->second == 0)
		{
			counts.erase(j);
		}
	}
	
	//words with no counts
	addedFactor[word] = exp(digamma(prior));

	double priorAdded = (_produced.size() * prior);
	total->second += priorAdded;
	total->second = exp(digamma(total->second));
	assert(maxCt < total->second);
	assert(addedFactor[word] < total->second);
}

void IBM::getContextWords(Sent* sent, NounContext& res, bool add)
{
}

void IBM::getProduced(Sent* sent, NounContexts& res, bool add)
{
	foreach(NPs, np, sent->nps())
	{
		int vind = _vocab.get((*np)->headSym(), add);
		res.push_back(NounContext(vind, (*np)->node()));
	}
}

Prob IBM::wordGivenPrevWord(NounContext& next, ContextWord& prevWord, 
						    bool train,
							Prob totAlign, MaxEntContext& context)
{
	const int nextWord = next.word;
	Prob alignProb;
	if(train && !_estimating)
	{
		alignProb = 1.0/totAlign;

		//if(prevWord.word < _nulls)
		{
			getRandBias(prevWord.word, nextWord);
		}
	}
	else
	{
		alignProb = _alignment.probToken(prevWord.index, context);
	}

	Prob wordProb = transProb(next, prevWord);
	Prob res = wordProb * alignProb;
// 	cerr<<_vocab.inv(next.word)<<" "<<_vocab.inv(prevWord.word)<<
// 		" word term "<<wordProb<<" alt term "<<alignProb<<"\n";

	return res;
}

Prob IBM::transProb(NounContext& next, ContextWord& prev)
{
	return transProbHelper(next.word, prev.word, _transCounts, 
						   _transTotals, _addedFactor);
}

Prob IBM::transProbHelper(int nextWord, int prevWord,
						  intIntProbMap* counts, intProbMap* totals,
						  intProbMap& addedFactor)
{
	Prob tot = (*totals)[prevWord];

	if(tot == 0)
	{
		return 1e-50;
	}

	Prob count = 0;
	intProbMap& pwCounts = (*counts)[prevWord];
	intProbMap::iterator entry = pwCounts.find(nextWord);
	if(entry != pwCounts.end())
	{
		count = entry->second;
	}
	else
	{
		//use "addedFactor" as count for items with 0-count
		count = addedFactor[prevWord];
	}

	Prob res = count / tot;
	if(!isProb(res))
	{
		cerr<<_vocab.inv(nextWord)<<" "<<nextWord<<" "<<
			_vocab.inv(prevWord)<<" "<<prevWord<<" "<<
			count<<" "<<tot<<" "<<res<<"\n";
	}
	assertProb(res);
	return res;
}

#include "PronounIBM.h"
#include "ECIBM.h"
#include "WordIBM.h"

IBM* IBM::create(string flag)
{
	IBM* model = NULL;

	if(flag == "-wp")
	{
		int prevS = 2;
		int nulls = 1;
		
		cerr<<"Making pronoun model with"<<prevS<<
			" context sentences, "<<nulls<<" nulls.\n";
		model = new PronounIBM(prevS);
	}
	else if(flag == "-ww")
	{
		double smooth = 0.1;
		int nulls = 1;
		int prevS = 1;
		
		cerr<<"Making IBM model with "<<prevS<<" context sentences, "<<
			nulls<<" topics, "<<
			"emission prior "<<smooth<<".\n";
		model = new WordIBM(prevS, smooth, nulls);
	}
	else if(flag == "-ww2")
	{
		double smooth = 0.1;
		int nulls = 1;
		int prevS = 2;
		
		cerr<<"Making IBM model with "<<prevS<<" context sentences, "<<
			nulls<<" topics, "<<
			"emission prior "<<smooth<<".\n";
		model = new WordIBM(prevS, smooth, nulls);
	}
	else
	{
		cerr<<"Unrecognized IBM flag "<<flag<<"\n";
		abort();
	}

	return model;
}

IBM* IBM::create(string flag, IBM* other)
{
	IBM* model = NULL;

	if(flag == "-wp")
	{
		cerr<<"Making pronoun same-head aligned IBM model.\n";
		model = new PronounIBM(*other);
	}
	else if(flag == "-ww")
	{
		int sents = 1;
		cerr<<"Making IBM model with "<<sents<<" sents.\n";
		model = new WordIBM(sents, *other);
	}
	else if(flag == "-ww2")
	{
		int sents = 2;
		cerr<<"Making IBM model with "<<sents<<" sents.\n";
		model = new WordIBM(sents, *other);
	}
	else
	{
		cerr<<"Unrecognized IBM flag "<<flag<<"\n";
		abort();
	}

	return model;
}

IBM* IBM::create(string flag, istream& is)
{
	IBM* model = NULL;

	if(flag == "-wp")
	{
		cerr<<"Reading pronoun same-head aligned IBM model from file...\n";
		model = new PronounIBM(is);
	}
	else if(flag == "-ww" || flag == "-ww2")
	{
		cerr<<"Making IBM model from file...\n";
		model = new WordIBM(is);
	}
	else if(flag == "-ec")
	{
		cerr<<"Reading Charniak model from file...\n";
		model = new ECIBM(is);
	}
	else
	{
		cerr<<"Unrecognized IBM flag "<<flag<<"\n";
		abort();
	}

	return model;
}

bool operator<(const NounContext& p1, const NounContext& p2)
{
	return p1.tree < p2.tree;
}
