#include "Disentangler.h"

Disentangler::Disentangler(CoherenceModel* model, 
						   Transcript* doc, bool verbose):
	_model(model),
	_transcript(doc),
	_verbose(verbose)
{
}

Disentangler::~Disentangler()
{
}

Prob Disentangler::objective()
{
	Prob res = 0;
	foreach(intDocumentMap, thread, _transcript->threads())
	{
		Prob term = _model->logProbability(*(thread->second));
// 		cerr<<"scoring "<<thread->first<<" "<<term<<"\n";
// 		cerr<<**thread<<"\n";
		res += term;
	}
	return res;
}

void Disentangler::solve()
{
	reset(); //XXX starts in true configuration if turned off

	while(true)
	{
		Prob obj = objective();
		cerr<<"Current objective "<<obj<<"\n";
		int best = -1;
		Prob bestGain = 0;
		int sents = _transcript->size();

		for(int sent = 0; sent < sents; ++sent)
		{
			_transcript->swap(sent);
			Prob gain = objective() - obj;
//			cerr<<"Moved "<<sent<<" for gain "<<gain<<"\n";
			if(gain > bestGain || sent == 0)
			{
				bestGain = gain;
				best = sent;
			}
			_transcript->swap(sent);
		}

		cerr<<"Best gain found: "<<bestGain<<" moving "<<best<<" "<<
			(*_transcript)[best]->index()<<"\n";
		if(bestGain > 0)
		{
			_transcript->swap(best);
		}
		else
		{
			break;
		}
	}
}

void Disentangler::reset()
{
	int sents = _transcript->size();

	for(int sent = 0; sent < sents; ++sent)
	{
		int targ = gsl_rng_uniform_int(RNG, 2);
		_transcript->move(sent, targ);
	}
}

Prob Disentangler::rank()
{
// 	int tests = 1000;
// 	int wins = 0;

// 	Prob original = objective();

// 	for(int tst = 0; tst < tests; ++tst)
// 	{
// 		if(tst % 100 == 0)
// 		{
// 			cerr<<"Test: "<<tst<<" rank "<<((Prob)wins/tst)<<"\n";
// 		}

// 		reset();

// 		Prob obj = objective();
// //		cerr<<"\tObj "<<obj<<"\n";
// 		if(original > obj)
// 		{
// 			++wins;
// 		}
// 	}
// 	return ((Prob)wins)/tests;

	int wins = 0;
	Prob obj = objective();
	int sents = _transcript->size();
	cerr<<"sents "<<sents<<"\n";

	for(int sent = 0; sent < sents; ++sent)
	{
		_transcript->swap(sent);
		Prob gain = objective() - obj;
		//cerr<<"Moved "<<sent<<" for gain "<<gain<<"\n";
		if(gain < 0)
		{
			wins++;
		}
		else if(gain > 0)
		{
			cerr<<"Move for "<<gain<<":\n";
			cerr<<*((*_transcript)[sent])<<"\n";
		}

		_transcript->swap(sent);
	}

	return ((Prob)wins)/sents;
}

Prob Disentangler::approxRank()
{
	ProbMat threadBySent;
	Probs threadGlobal;
	foreach(intDocumentMap, thread, _transcript->threads())
	{
		_model->logProbability(*thread->second);
		assert(!_model->sentScores().empty());
		threadBySent.push_back(_model->sentScores());
		threadGlobal.push_back(_model->globalScore());
	}

	int wins = 0;
	int ties = 0;
	int sents = _transcript->size();
	cerr<<"Transcript length: "<<sents<<"\n";

	int hist = _model->history();

	int ti = 0;
	foreach(intDocumentMap, thread, _transcript->threads())
	{
//		cerr<<"Begin to handle thread "<<ti<<"\n";
		Probs& origScores = threadBySent[ti];
		Probs& otherScores = threadBySent[1 - ti];

		Document& thisThread = *(thread->second);
		Document& otherThread = _transcript->thread(1 - ti);

		int threadSize = thisThread.size();
		int otherThreadSize = otherThread.size();

// 		cerr<<"This thread has size "<<threadSize<<
// 			" and other thread has "<<otherThreadSize<<"\n";

		for(int sent = 0; sent < threadSize; ++sent)
		{
			Sent* currSent = thisThread[sent];
			int whereWent = _transcript->swap(currSent->index());

// 			cerr<<"Moving sent "<<sent<<" from thread "<<ti<<
// 				" to index "<<whereWent<<"\n";

			Prob gainTo = 0;

			//score in thread we moved it to
			{
				ints region;
				int rBegin = whereWent - hist;
				if(rBegin < 0)
				{
					rBegin = 0;
				}
				int rEnd = whereWent + hist + 1;
				//other thread is now one larger because of the move
				if(rEnd > otherThreadSize + 1)
				{
					rEnd = otherThreadSize + 1;
				}
				for(int ii = rBegin; ii < rEnd; ++ii)
				{
//					cerr<<"Region in target: "<<ii<<"\n";
					region.push_back(ii);
				}
				_model->initCaches(otherThread);
				_model->permProbability(otherThread, region, false);
				_model->clearCaches();
				Probs& newScores = _model->sentScores();
				int nScores = newScores.size();
				for(int ii = whereWent - rBegin; ii < nScores; ++ii)
				{
// 					cerr<<"Scoring "<<rBegin + ii<<
// 						" "<<newScores[ii]<<"\n";
					gainTo += newScores[ii];
				}
//				cerr<<"New global term "<<_model->globalScore()<<"\n";
				gainTo += _model->globalScore();

				for(int ii = whereWent; 
					ii < whereWent + hist && ii < otherThreadSize;
					++ii)
				{
// 					cerr<<"Corresponding scoring without: "<<ii<<
// 						" "<<origScores[ii]<<"\n";
					gainTo -= otherScores[ii];
				}
//				cerr<<"Old global term "<<threadGlobal[ti]<<"\n";
				gainTo -= threadGlobal[ti];

			}

//			cerr<<" Gain in target thread "<<gainTo<<"\n";

			Prob gainFrom = 0;

			//score in this thread (where we took it from)
			{
				ints region;
				int rBegin = sent - hist;
				if(rBegin < 0)
				{
					rBegin = 0;
				}
				int rEnd = sent + hist;
				if(rEnd > threadSize - 1)
				{
					rEnd = threadSize - 1;
				}
				for(int ii = rBegin; ii < rEnd; ++ii)
				{
//					cerr<<"Region in original: "<<ii<<"\n";
					region.push_back(ii);
				}
				_model->initCaches(thisThread);
				_model->permProbability(thisThread, region, false);
				_model->clearCaches();
				Probs& newScores = _model->sentScores();
				int nScores = newScores.size();
				for(int ii = sent - rBegin; ii < nScores; ++ii)
				{
// 					cerr<<"Scoring "<<rBegin + ii<<
// 						" "<<newScores[ii]<<"\n";
					gainFrom += newScores[ii];
				}
//				cerr<<"New global term "<<_model->globalScore()<<"\n";
				gainFrom += _model->globalScore();

				for(int ii = sent;
					ii < sent + hist + 1 && ii < threadSize;
					++ii)
				{
// 					cerr<<"Corresponding scoring without: "<<ii<<
// 						" "<<origScores[ii]<<"\n";
					gainFrom -= origScores[ii];
				}
//				cerr<<"Old global term "<<threadGlobal[1 - ti]<<"\n";
				gainFrom -= threadGlobal[1 - ti];

			}

//			cerr<<" Gain in original thread "<<gainFrom<<"\n";

			Prob gain = gainTo + gainFrom;

//			cerr<<"Total gain "<<gain<<"\n\n";

			if(gain < 0)
			{
				wins++;

// 				cerr<<"Don't move for "<<gain<<":\n";
// 				cerr<<*currSent<<"\n";
			}
			else if(gain > 0)
			{
				cerr<<"Move for "<<gain<<":\n";
				cerr<<*currSent<<"\n";
			}
			else
			{
				assert(gain == 0);
				++ties;
			}

			int where = _transcript->swap(currSent->index());
			assert(where == sent);
		}

		++ti;
	}

	cerr<<ties<<" ties.\n";
	return ((Prob)wins)/sents;
}

void Disentangler::tabooSolve()
{
	reset(); //XXX starts in true configuration if turned off

	intsLst states;
	intsSet taboo;

	int sinceImproved = 0;

	Prob best = 1;
	ints bestConfig;

	evaluateMoves();

	Prob obj = objective();

	int steps = 0;
	while(true)
	{
		++steps;
		if(steps % 50 == 0)
		{
			cerr<<"Step "<<steps<<": Reevaluating moves...\n";
			evaluateMoves(); //approximate objective drifts a little...
			obj = objective();
		}
		cerr<<"Current objective "<<obj<<"\n";

		++sinceImproved;
		if(best == 1 || obj > best)
		{
			sinceImproved = 0;
			best = obj;
			bestConfig.resize(_transcript->size());
			foreach(Transcript, sent, *_transcript)
			{
				bestConfig[(*sent)->index()] = (*sent)->dialogue();
			}
		}	

		Prob gain;
		int targ = -1;
		int move = bestMove(gain, taboo, targ);

		cerr<<"Moving "<<move<<" for gain "<<gain<<"\n";

		if(sinceImproved == 1000)
		{
			break;
		}
		if(steps > 500)
		{
			break;
		}

		int threadFrom = ((*_transcript)[move])->dialogue();
		int whereFrom = _transcript->take(move);
		int where = _transcript->put(move, 1 - threadFrom);
		obj = objective();
		assert(targ == 1 - threadFrom);

		recalc(whereFrom, where, threadFrom, 1 - threadFrom);
//		evaluateMoves();

		ints config(_transcript->size());
		foreach(Transcript, sent, *_transcript)
		{
			config[(*sent)->index()] = (*sent)->dialogue();
		}

		taboo.insert(config);
		states.push_back(config);
		if(states.size() > 500)
		{
			ints& prev = states.front();
			taboo.erase(prev);
			states.pop_front();
		}
	}

	int sents = _transcript->size();
	for(int sent = 0; sent < sents; ++sent)
	{
		_transcript->move(sent, bestConfig[sent]);
	}

	cerr<<"Best objective "<<objective()<<"\n";
}

int Disentangler::bestMove(Prob& bestGain, intsSet& taboo, int& targetThread)
{
	int best = -1;
	int size = _transcript->size();
	ints config(size);
	foreach(Transcript, sent, *_transcript)
	{
		config[(*sent)->index()] = (*sent)->dialogue();
	}

	int phase = 0;
	//on short test documents, may randomly check 0 moves, so loop and
	//check all on 2nd pass
	while(best == -1)
	{
		for(int ii = 0; ii < size; ++ii)
		{
			Sent* currSent = (*_transcript)[ii];
			assert(currSent->index() == ii);

			//don't allow any fixed sentence to move
			if(contains(_fixed, ii))
			{
				continue;
			}

//			foreach(intDocumentMap, target, _transcript->threads())

			intProbMap& moveTab = _moves[ii];
			foreach(intProbMap, target, moveTab)
			{
//				cerr<<ii<<" "<<target->first<<"\n";
				if(target->first == currSent->dialogue())
				{
					continue;
				}

				//check a random 1/5th of moves?
				if(phase == 0 && gsl_rng_uniform_int(RNG, 5) != 0)
				{
					continue;
				}

//				Prob currGain = _moves[ii][target->first];
				Prob currGain = target->second;

// 				cerr<<"Moving "<<ii<<" to "<<target->first<<
// 					" gains "<<currGain<<"\n";

				if(best == -1 || currGain > bestGain)
				{
					int oldState = config[ii];
					config[ii] = target->first;

					if(phase > 1 || !contains(taboo, config))
					{
						best = ii;
						bestGain = currGain;
						targetThread = target->first;
					}

					config[ii] = oldState;
				}
			}
		}

		++phase;
	}

	return best;
}

void Disentangler::recalc(int sent, int whereWent, int ti, int tTo)
{
	assert(tTo == 1 - ti);

	Document& thisThread = _transcript->thread(ti);
	Document& otherThread = _transcript->thread(1 - ti);

	Probs& origScores = _threadBySent[ti];
	Probs& otherScores = _threadBySent[1 - ti];

	int threadSize = thisThread.size();
	int otherThreadSize = otherThread.size();

	int hist = _model->history();

	//score in thread we moved it to
	{
		ints region;
		int rBegin = whereWent - hist;
		if(rBegin < 0)
		{
			rBegin = 0;
		}
		int rEnd = whereWent + hist + 1;
		//other thread is now one larger because of the move
		if(rEnd > otherThreadSize)
		{
			rEnd = otherThreadSize;
		}
		for(int ii = rBegin; ii < rEnd; ++ii)
		{
//			cerr<<"Region in target: "<<ii<<"\n";
			region.push_back(ii);
		}
		_model->initCaches(otherThread);
		_model->permProbability(otherThread, region, false);
		_model->clearCaches();
		Probs& newScores = _model->sentScores();
		_threadGlobal[1 - ti] = _model->globalScore();
		Probs::iterator iPos = otherScores.begin();
		iPos += whereWent;
		otherScores.insert(iPos, newScores[whereWent - rBegin]);
		for(int ii = whereWent + 1; ii < rEnd; ++ii)
		{
			otherScores[ii] = newScores[ii - rBegin];
		}
	}

	//score in this thread (where we took it from)
	{
		ints region;
		int rBegin = sent - hist;
		if(rBegin < 0)
		{
			rBegin = 0;
		}
		int rEnd = sent + hist;
		if(rEnd > threadSize)
		{
			rEnd = threadSize;
		}
		for(int ii = rBegin; ii < rEnd; ++ii)
		{
//			cerr<<"Region in original: "<<ii<<"\n";
			region.push_back(ii);
		}
		_model->initCaches(thisThread);
		_model->permProbability(thisThread, region, false);
		_model->clearCaches();
		Probs& newScores = _model->sentScores();
		_threadGlobal[ti] = _model->globalScore();
		Probs::iterator iPos = origScores.begin();
		iPos += sent;
		origScores.erase(iPos);
		for(int ii = sent; ii < rEnd; ++ii)
		{
			origScores[ii] = newScores[ii - rBegin];
		}
	}

	{
		int rBegin = whereWent - hist;
		if(rBegin < 0)
		{
			rBegin = 0;
		}
		int rEnd = whereWent + hist + 1;
		//other thread is now one larger because of the move
		if(rEnd > otherThreadSize)
		{
			rEnd = otherThreadSize;
		}
		for(int ii = rBegin; ii < rEnd; ++ii)
		{
			int currInd = otherThread[ii]->index();
			_moves[currInd][ti] = gainByMoving(ii, 1 - ti, ti);
		}
	}

	{
		int rBegin = sent - hist;
		if(rBegin < 0)
		{
			rBegin = 0;
		}
		int rEnd = sent + hist;
		if(rEnd > threadSize)
		{
			rEnd = threadSize;
		}
		for(int ii = rBegin; ii < rEnd; ++ii)
		{
			int currInd = thisThread[ii]->index();
			_moves[currInd][1 - ti] = gainByMoving(ii, ti, 1 - ti);
		}
	}
}

void Disentangler::evaluateMoves()
{
//	cerr<<"Transcript length: "<<sents<<"\n";

	_threadBySent.clear();
	_threadGlobal.clear();
	foreach(intDocumentMap, thread, _transcript->threads())
	{
		_model->logProbability(*(thread->second));
		assert(_model->sentScores().size() == thread->second->size());
		_threadBySent[thread->first] = _model->sentScores();
		_threadGlobal[thread->first] = _model->globalScore();
	}

	int ti = 0;
	foreach(intDocumentMap, thread, _transcript->threads())
	{
//		cerr<<"Begin to handle thread "<<ti<<"\n";
// 		cerr<<"This thread has size "<<threadSize<<
// 			" and other thread has "<<otherThreadSize<<"\n";

		Document& currThread = *(thread->second);
		int threadSize = currThread.size();

		for(int sent = 0; sent < threadSize; ++sent)
		{
			Prob gain = gainByMoving(sent, ti, 1 - ti);

//			cerr<<"Move "<<sent<<" has gain "<<gain<<"\n";

			_moves[sent][1 - ti] = gain;
		}

		++ti;
	}
}

Prob Disentangler::gainByMoving(int sent, int ti, int targ)
{
	assert(targ == 1 - ti);
	int hist = _model->history();

	Probs& origScores = _threadBySent[ti];
	Probs& otherScores = _threadBySent[1 - ti];

	Document& thisThread = _transcript->thread(ti);
	Document& otherThread = _transcript->thread(1 - ti);

	int threadSize = thisThread.size();
	int otherThreadSize = otherThread.size();

	Sent* currSent = thisThread[sent];
	int whereWent = _transcript->swap(currSent->index());

// 	cerr<<"Moving sent "<<sent<<" from thread "<<ti<<
// 		" to index "<<whereWent<<"\n";

	Prob gainTo = 0;

	//score in thread we moved it to
	{
		ints region;
		int rBegin = whereWent - hist;
		if(rBegin < 0)
		{
			rBegin = 0;
		}
		int rEnd = whereWent + hist + 1;
		//other thread is now one larger because of the move
		if(rEnd > otherThreadSize + 1)
		{
			rEnd = otherThreadSize + 1;
		}
		for(int ii = rBegin; ii < rEnd; ++ii)
		{
//			cerr<<"Region in target: "<<ii<<"\n";
			region.push_back(ii);
		}
		_model->initCaches(otherThread);
		_model->permProbability(otherThread, region, false);
		_model->clearCaches();
		Probs& newScores = _model->sentScores();
		int nScores = newScores.size();
		for(int ii = whereWent - rBegin; ii < nScores; ++ii)
		{
// 			cerr<<"Scoring "<<rBegin + ii<<
// 				" "<<newScores[ii]<<"\n";
			gainTo += newScores[ii];
		}
		gainTo += _model->globalScore();
		for(int ii = whereWent; 
			ii < whereWent + hist && ii < otherThreadSize;
			++ii)
		{
// 			cerr<<"Corresponding scoring without: "<<ii<<
// 				" "<<origScores[ii]<<"\n";
			gainTo -= otherScores[ii];
		}
		gainTo -= _threadGlobal[ti];
	}

//	cerr<<" Gain in target thread "<<gainTo<<"\n";

	Prob gainFrom = 0;

	//score in this thread (where we took it from)
	{
		ints region;
		int rBegin = sent - hist;
		if(rBegin < 0)
		{
			rBegin = 0;
		}
		int rEnd = sent + hist;
		if(rEnd > threadSize - 1)
		{
			rEnd = threadSize - 1;
		}
		for(int ii = rBegin; ii < rEnd; ++ii)
		{
//					cerr<<"Region in original: "<<ii<<"\n";
			region.push_back(ii);
		}
		_model->initCaches(thisThread);
		_model->permProbability(thisThread, region, false);
		_model->clearCaches();
		Probs& newScores = _model->sentScores();
		int nScores = newScores.size();
		for(int ii = sent - rBegin; ii < nScores; ++ii)
		{
// 					cerr<<"Scoring "<<rBegin + ii<<
// 						" "<<newScores[ii]<<"\n";
			gainFrom += newScores[ii];
		}
		gainFrom += _model->globalScore();

		for(int ii = sent;
			ii < sent + hist + 1 && ii < threadSize;
			++ii)
		{
// 					cerr<<"Corresponding scoring without: "<<ii<<
// 						" "<<origScores[ii]<<"\n";
			gainFrom -= origScores[ii];
		}
		gainFrom -= _threadGlobal[1 - ti];
	}

//			cerr<<"Gain in original thread "<<gainFrom<<"\n";

	Prob gain = gainTo + gainFrom;

	assert(isfinite(gainTo));
	assert(isfinite(gainFrom));

	int where = _transcript->swap(currSent->index());
	assert(where == sent);

	return gain;
}
