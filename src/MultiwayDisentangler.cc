#include "MultiwayDisentangler.h"

MultiwayDisentangler::MultiwayDisentangler(CoherenceModel* model, 
										   Transcript* doc, bool verbose):
	Disentangler(model, doc, verbose)
{
	_transcript->setLocked(false);
}

void MultiwayDisentangler::solve()
{
	cerr<<"I don't plan to support this. Run tabooSolve\n";
	abort();
}

void MultiwayDisentangler::reset()
{
	int sents = _transcript->size();

	for(int sent = 0; sent < sents; ++sent)
	{
		_transcript->move(sent, sent);
	}
}

Prob MultiwayDisentangler::rank()
{
	int wins = 0;
	Prob obj = objective();
	int sents = _transcript->size();
	int ties = 0;
	int tests = 0;
	int sentWins = 0;
	cerr<<"sents "<<sents<<"\n";

	int emptyThread = _transcript->openThread();

	_transcript->setLocked(true);

	for(int sent = 0; sent < sents; ++sent)
	{
		Sent* curr = (*_transcript)[sent];
		cerr<<*curr<<"\n";
		Document& thisThread = _transcript->thread(curr->dialogue());
		int threadSize = thisThread.size();
		int trueThread = ((*_transcript)[sent])->dialogue();

		bool lost = false;

		foreach(intDocumentMap, other, _transcript->threads())
		{
			int thread = other->first;

			if(thread == trueThread)
			{
				continue;
			}

			if(thread == emptyThread && threadSize == 1)
			{
				continue;
			}

			++tests;
			_transcript->move(sent, thread);
			Prob gain = objective() - obj;
//			cerr<<"Moved "<<sent<<" to "<<thread<<" for gain "<<gain<<"\n";
			if(gain < 0)
			{
				wins++;
			}
			else if(gain > 0)
			{
				lost = true;
			}
			else
			{
				++ties;
			}

			_transcript->move(sent, trueThread);
		}
//		cerr<<"\n";

		if(!lost)
		{
			++sentWins;
		}
	}

	_transcript->setLocked(false);
	_transcript->closeThread(emptyThread);

	cerr<<ties<<" ties\n";
	cerr<<sentWins<<" "<<sents<<" "<<(sentWins/((Prob)sents))<<" sents\n";
	cerr<<wins<<" "<<tests<<" "<<((Prob)wins/tests)<<" wins/tests\n";
	return ((Prob)wins)/tests;
}

Prob MultiwayDisentangler::approxRank()
{
	int emptyThread = _transcript->openThread();
//	cerr<<"Empty thread "<<emptyThread<<"\n";
	foreach(intDocumentMap, thread, _transcript->threads())
	{
// 		cerr<<"Recording initial-configuration score for "<<
// 			thread->first<<"\n";

		_model->logProbability(*thread->second);
		if(!thread->second->empty())
		{
			assert(!_model->sentScores().empty());
		}

		_threadBySent[thread->first] = _model->sentScores();
		_threadGlobal[thread->first] = _model->globalScore();
	}

	int wins = 0;
	int tests = 0;
	int ties = 0;
	int sentWins = 0;
	int sents = _transcript->size();
	cerr<<"Transcript length: "<<sents<<"\n";

	_transcript->setLocked(true);

	foreach(intDocumentMap, thread, _transcript->threads())
	{
		int ti = thread->first;
//		cerr<<"Begin to handle thread "<<ti<<"\n";

		Document& thisThread = *(thread->second);
		int threadSize = thisThread.size();

		for(int sent = 0; sent < threadSize; ++sent)
		{
			bool lost = false;

			Sent* currSent = thisThread[sent];
			cerr<<"Processing:\n"<<*currSent<<"\n";

			foreach(intDocumentMap, other, _transcript->threads())
			{
				int target = other->first;
 				if(target == ti)
				{
					continue;
				}

				//this block is cheating, and should only be on in debug
// 				if(target == emptyThread)
// 				{
// 					continue;
// 				}

				if(target == emptyThread && threadSize == 1)
				{
					continue;
				}

				Prob gain = gainByMoving(sent, ti, target);

// 				cerr<<"Moved "<<currSent->index()<<" to thread "<<
// 					target<<" for gain "<<gain<<"\n";

				if(gain < 0)
				{
					wins++;
//					cerr<<"Don't move for "<<gain<<"\n";
				}
				else if(gain > 0)
				{
//					cerr<<"Move to "<<target<<" for "<<gain<<"\n";
					lost = true;
				}
				else
				{
					assert(gain == 0);
					++ties;
				}
				++tests;
			}

			if(!lost)
			{
				++sentWins;

				cerr<<"WIN\n";

// 				cout<<"T1 "<<currSent->time()<<" "<<"S"<<
// 					currSent->speaker()<<" :  "<<
// 					Sent::plaintext(currSent->tree())<<"\n";
			}
			else
			{
				cerr<<"LOSE\n";

// 				cout<<"T2 "<<currSent->time()<<" "<<"S"<<
// 					currSent->speaker()<<" :  "<<
// 					Sent::plaintext(currSent->tree())<<"\n";
			}
		}
	}

	_transcript->setLocked(false);
	_transcript->closeThread(emptyThread);

	cerr<<ties<<" ties.\n";
	cerr<<"Correctly decided "<<(((Prob)wins)/tests)<<" tests ("<<
		wins<<" of "<<tests<<")\n";
	cerr<<"Completely correct on "<<(((Prob)sentWins)/sents)<<" tests ("<<
		sentWins<<" of "<<sents<<")\n";

	return ((Prob)wins)/tests;
}

Prob MultiwayDisentangler::gainByMoving(int sent, int ti, int target)
{
	int hist = _model->history();

	Probs& origScores = _threadBySent[ti];
	Document& thisThread = _transcript->thread(ti);
	int threadSize = thisThread.size();

	Probs& otherScores = _threadBySent[target];
	Document& otherThread = _transcript->thread(target);
	int otherThreadSize = otherThread.size();

	Sent* currSent = thisThread[sent];
	int whereWent = _transcript->move(currSent->index(), target);

	const bool verbose = false;

	if(verbose)
	{
		cerr<<"Moving sent "<<sent<<" from thread "<<ti<<
			" to index "<<whereWent<<" in "<<target<<"\n";
	}

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
			if(verbose)
			{
				cerr<<"Region in target: "<<ii<<"\n";
			}
			region.push_back(ii);
		}
		_model->initCaches(otherThread);
		_model->permProbability(otherThread, region, false);
		_model->clearCaches();
		Probs& newScores = _model->sentScores();
		int nScores = newScores.size();
		for(int ii = whereWent - rBegin; ii < nScores; ++ii)
		{
			if(verbose)
			{
				cerr<<"Scoring "<<rBegin + ii<<" "<<newScores[ii]<<"\n";
			}
			gainTo += newScores[ii];
		}
		if(verbose)
		{
			cerr<<"New global term "<<_model->globalScore()<<"\n";
		}
		gainTo += _model->globalScore();

		for(int ii = whereWent; 
			ii < whereWent + hist && ii < otherThreadSize;
			++ii)
		{
			if(verbose)
			{
				cerr<<"Corresponding scoring without: "<<ii<<
					" "<<origScores[ii]<<"\n";
			}
			gainTo -= otherScores[ii];
		}
		if(verbose)
		{
			cerr<<"Old global term "<<_threadGlobal[ti]<<"\n";
		}
		gainTo -= _threadGlobal[ti];

	}

	if(verbose)
	{
		cerr<<" Gain in target thread "<<gainTo<<"\n";
	}

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
			if(verbose)
			{
				cerr<<"Region in original: "<<ii<<"\n";
			}
			region.push_back(ii);
		}
		_model->initCaches(thisThread);
		_model->permProbability(thisThread, region, false);
		_model->clearCaches();
		Probs& newScores = _model->sentScores();
		int nScores = newScores.size();
		for(int ii = sent - rBegin; ii < nScores; ++ii)
		{
			if(verbose)
			{
				cerr<<"Scoring "<<rBegin + ii<<" "<<newScores[ii]<<"\n";
			}
			gainFrom += newScores[ii];
		}
		if(verbose)
		{
			cerr<<"New global term "<<_model->globalScore()<<"\n";
		}
		gainFrom += _model->globalScore();

		for(int ii = sent;
			ii < sent + hist + 1 && ii < threadSize;
			++ii)
		{
			if(verbose)
			{
				cerr<<"Corresponding scoring without: "<<ii<<
					" "<<origScores[ii]<<"\n";
			}
			gainFrom -= origScores[ii];
		}
		if(verbose)
		{
			cerr<<"Old global term "<<_threadGlobal[target]<<"\n";
		}
		gainFrom -= _threadGlobal[target];
	}

	if(verbose)
	{
		cerr<<" Gain in original thread "<<gainFrom<<"\n";
	}

	Prob gain = gainTo + gainFrom;

	if(verbose)
	{
		cerr<<"Total gain "<<gain<<"\n";
	}

	int where = _transcript->move(currSent->index(), ti);
//	cerr<<"Restored to "<<where<<" in "<<ti<<"\n";
	assert(where == sent);

	return gain;
}

void MultiwayDisentangler::evaluateMoves()
{
	_transcript->setLocked(false);
	int emptyThread = _transcript->openThread();
//	cerr<<"Empty thread "<<emptyThread<<"\n";
	foreach(intDocumentMap, thread, _transcript->threads())
	{
// 		cerr<<"Recording initial-configuration score for "<<
// 			thread->first<<"\n";

		_model->logProbability(*thread->second);
		if(!thread->second->empty())
		{
			assert(!_model->sentScores().empty());
		}

		_threadBySent[thread->first] = _model->sentScores();
		_threadGlobal[thread->first] = _model->globalScore();
	}

	_transcript->setLocked(true);

	foreach(intDocumentMap, thread, _transcript->threads())
	{
		int ti = thread->first;
		cerr<<"Thread "<<ti<<"\n";

		Document& thisThread = *(thread->second);
		int threadSize = thisThread.size();

		for(int sent = 0; sent < threadSize; ++sent)
		{
			cerr<<sent<<" ";

			Sent* currSent = thisThread[sent];
//			cerr<<"Processing:\n"<<*currSent<<"\n";

			foreach(intDocumentMap, other, _transcript->threads())
			{
				int target = other->first;
				if(target == ti)
				{
					continue;
				}
				if(target == emptyThread && threadSize == 1)
				{
					continue;
				}

				Prob gain = gainByMoving(sent, ti, target);
// 				cerr<<"Moving "<<sent<<" from "<<
// 					ti<<" to "<<target<<": "<<gain<<"\n";
				_moves[currSent->index()][target] = gain;
			}
		}
		cerr<<"\n";
	}

	_transcript->setLocked(false);
	_transcript->closeThread(emptyThread);
	_transcript->setLocked(true);
}

Prob MultiwayDisentangler::partialObjective(int sLast, int emptyThread)
{
	Prob res = 0;
	for(int thr = 0; thr <= emptyThread; ++thr)
	{
		Document& thread = _transcript->thread(thr);
		ints subsection;
		int threadSize = thread.size();
		for(int si = 0; si < threadSize; ++si)
		{
			if(thread[si]->index() <= sLast)
			{
				subsection.push_back(si);
			}
		}
		_model->initCaches(thread);
		Prob term = _model->permProbability(thread, subsection, false);
		_model->clearCaches();

		res += term;
	}
	return res;
}

Prob MultiwayDisentangler::partialObjective(int sLast, intSet& live)
{
	Prob res = 0;
	foreach(intSet, thr, live)
	{
		Document& thread = _transcript->thread(*thr);
		ints subsection;
		int threadSize = thread.size();
		for(int si = 0; si < threadSize; ++si)
		{
			if(thread[si]->index() <= sLast)
			{
				subsection.push_back(si);
			}
		}
		_model->initCaches(thread);
		Prob term = _model->permProbability(thread, subsection, false);
		_model->clearCaches();

		res += term;
	}
	return res;
}

void MultiwayDisentangler::greedySolve()
{
	_transcript->setLocked(false);

	int sents = _transcript->size();
	for(int si = 0; si < sents; ++si)
	{
		_transcript->move(si, si);
	}

	_transcript->setLocked(true);

	int emptyThread = 0;
	for(int si = 0; si < sents; ++si)
	{
		cerr<<si<<"****************\n";
		int currTime = (*_transcript)[si]->time();
		int best = -1;
		Prob bestObj = 0;
		for(int thr = 0; thr <= emptyThread; ++thr)
		{
			Document& targ = _transcript->thread(thr);
			if(!targ.empty())
			{
				int deltaT = currTime - (*targ.rbegin())->time();
				if(deltaT > 129)
				{
					continue;
				}
			}

			_transcript->move(si, thr);
			Prob partObj = partialObjective(si, emptyThread);
			cerr<<"\t"<<thr<<" "<<partObj<<"\n";
			if(best == -1 || partObj > bestObj)
			{
				bestObj = partObj;
				best = thr;
			}
		}
		_transcript->move(si, best);
		cerr<<"Put "<<si<<" in "<<best<<"\n";
		if(best == emptyThread)
		{
			++emptyThread;
//			cerr<<"Update empty thread to "<<emptyThread<<"\n";
		}
	}
}

void MultiwayDisentangler::greedyPartialSolve()
{
	int sents = _transcript->size();

//	int emptyThread = _transcript->openThread();
	_transcript->setLocked(true);

	intSet liveThreads;
//	liveThreads.insert(emptyThread);
	for(int si = 0; si < sents; ++si)
	{
		if((*_transcript)[si]->dialogue() != 0)
		{
			_fixed.insert(si);
			liveThreads.insert((*_transcript)[si]->dialogue());
			continue;
		}

		cerr<<si<<"****************\n";
		int currTime = (*_transcript)[si]->time();
		int best = -1;
		Prob bestObj = 0;
		foreach(intSet, thr, liveThreads)
		{
			Document& targ = _transcript->thread(*thr);
			if(!targ.empty())
			{
				int deltaT = currTime - (*targ.rbegin())->time();
				if(deltaT > 129)
				{
					continue;
				}
			}

			_transcript->move(si, *thr);
			Prob partObj = partialObjective(si, liveThreads);
			cerr<<"\t"<<*thr<<" "<<partObj<<"\n";
			if(best == -1 || partObj > bestObj)
			{
				bestObj = partObj;
				best = *thr;
			}
		}
		if(best != -1)
		{
			_transcript->move(si, best);
		}
		else
		{
			_transcript->setLocked(false);
			int empty = _transcript->openThread();
			liveThreads.insert(empty);
			_transcript->setLocked(true);
			_transcript->move(si, empty);
		}
		cerr<<"Put "<<si<<" in "<<best<<"\n";
	}
}

void MultiwayDisentangler::tabooSolve()
{
	//no reset-- make sure to use other system to find appropriate initial
	//state

	//	reset();

	_transcript->setLocked(true);

	intsLst states;
	intsSet taboo;

	int sinceImproved = 0;

	Prob best = 1;
	ints bestConfig(_transcript->size());

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
			foreach(Transcript, sent, *_transcript)
			{
				bestConfig[(*sent)->index()] = (*sent)->dialogue();
			}
		}

		Prob gain;
		int targ = -1;
		int move = bestMove(gain, taboo, targ);
		if(!inMap(_transcript->threads(), targ))
		{
			_moves[move].erase(targ);
			_transcript->setLocked(false);
			targ = _transcript->openThread();
			_transcript->setLocked(true);
		}

//		cerr<<"Moving "<<move<<" to "<<targ<<" for gain "<<gain<<"\n";

		if(sinceImproved == 1000)
		{
			break;
		}
		if(steps > 498)
		{
			break;
		}

		int threadFrom = ((*_transcript)[move])->dialogue();
		int whereFrom = _transcript->take(move);
		int where = _transcript->put(move, targ);

		cerr<<"Moved "<<move<<" from "<<whereFrom<<" in "<<threadFrom<<
			" to "<<where<<" in "<<targ<<" for gain "<<gain<<"\n";
		assert((*_transcript)[move]->index() == move);

		obj = objective();

		recalc(whereFrom, where, threadFrom, targ);

		if(_transcript->thread(threadFrom).empty())
		{
			_transcript->setLocked(false);
			_transcript->closeThread(threadFrom);
			_transcript->setLocked(true);

			_threadBySent.erase(threadFrom);
			_threadGlobal.erase(threadFrom);
			int size = _transcript->size();
			for(int sent = 0; sent < size; ++sent)
			{
				_moves[sent].erase(threadFrom);
			}
		}
			
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

	_transcript->setLocked(false);

	int sents = _transcript->size();
	for(int sent = 0; sent < sents; ++sent)
	{
		_transcript->move(sent, bestConfig[sent]);
	}

	cerr<<"Best objective "<<objective()<<"\n";
}

void MultiwayDisentangler::recalc(int sent, int whereWent, int tFrom, int tTo)
{
// 	cerr<<"Recalculating relationships for "<<sent<<" from "<<
// 		tFrom<<" becoming "<<whereWent<<" in "<<tTo<<"\n";

	Document& thisThread = _transcript->thread(tFrom);
	Document& otherThread = _transcript->thread(tTo);

	Probs& origScores = _threadBySent[tFrom];
	Probs& otherScores = _threadBySent[tTo];

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
		_threadGlobal[tTo] = _model->globalScore();
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
		_threadGlobal[tFrom] = _model->globalScore();
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
			_moves[currInd][tFrom] = gainByMoving(ii, tTo, tFrom);
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
			_moves[currInd][tTo] = gainByMoving(ii, tFrom, tTo);
		}
	}

	if(otherThreadSize == 2)
	{
		//calculate movements to new thread
		_transcript->setLocked(false);
		int empty = _transcript->openThread();
		_transcript->setLocked(true);

		for(int ii = 0; ii < otherThreadSize; ++ii)
		{
			int currInd = otherThread[ii]->index();
			_moves[currInd][empty] = gainByMoving(ii, tTo, empty);
		}

		_transcript->setLocked(false);
		_transcript->closeThread(empty);
		_transcript->setLocked(true);		
	}

	//dammit, recalculate everything; clean up later
	int tFromBegin = sent - hist;
	if(threadSize == 0)
	{
		tFromBegin = -1;
	}
	else if(tFromBegin < 0)
	{
		tFromBegin = 0;
	}
	else
	{
		tFromBegin = thisThread[tFromBegin]->index();
	}

	//convert region marker to absolute index
	int tFromEnd = sent + hist;
	if(threadSize == 0)
	{
		tFromEnd = -1;
	}
	else if(tFromEnd > threadSize)
	{
		tFromEnd = _transcript->size();
	}
	else
	{
		tFromEnd = thisThread[tFromEnd - 1]->index();
	}

	int tToBegin = whereWent - hist;
	if(tToBegin < 0)
	{
		tToBegin = 0;
	}
	else
	{
		tToBegin = otherThread[tToBegin]->index();
	}
	int tToEnd = whereWent + hist + 1;
	if(tToEnd > otherThreadSize)
	{
		tToEnd = _transcript->size();
	}
	else
	{
		tToEnd = otherThread[tToEnd - 1]->index();
	}

	foreach(intDocumentMap, thirdThread, _transcript->threads())
	{
		if(thirdThread->first == tFrom || thirdThread->first == tTo)
		{
			continue;
		}

//		cerr<<"Looking at moves from "<<thirdThread->first<<"\n";
		int ttSize = thirdThread->second->size();

		for(int possMove = 0; possMove < ttSize; ++possMove)
		{
			Sent* curr = (*thirdThread->second)[possMove];
// 			cerr<<possMove<<" is "<<curr->index()<<" bounds "<<
// 				tFromBegin<<" "<<tFromEnd<<" "<<tToBegin<<" "<<tToEnd<<"\n";

			if(curr->index() > tFromBegin && curr->index() < tFromEnd)
			{
// 				cerr<<"Looking at move "<<possMove<<" from "<<
// 					thirdThread->first<<" to "<<tFrom<<"\n";
				_moves[curr->index()][tFrom] = 
					gainByMoving(possMove, thirdThread->first, tFrom);
			}

			if(curr->index() > tToBegin && curr->index() < tToEnd)
			{
// 				cerr<<"Looking at move "<<possMove<<" from "<<
// 					thirdThread->first<<" to "<<tTo<<"\n";
				_moves[curr->index()][tTo] = 
					gainByMoving(possMove, thirdThread->first, tTo);
			}
		}
	}
}

