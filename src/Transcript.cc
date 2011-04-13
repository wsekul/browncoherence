#include "Transcript.h"

Transcript::Transcript(string& name):
	Document(name),
	_threadsLocked(false),
	_nextInd(0)
{
	setAll();

	setLocked(true);
}

Transcript::Transcript(char* name):
	Document(name),
	_threadsLocked(false),
	_nextInd(0)
{
	setAll();

	setLocked(true);
}

Transcript::Transcript(istream& is, string& name):
	Document(is, name),
	_threadsLocked(false),
	_nextInd(0)
{
	setLocked(false);

	setAll();

	setLocked(true);
}

Transcript::~Transcript()
{
	foreach(intDocumentMap, thread, _threads)
	{
		thread->second->clear();
		delete thread->second;
	}
}

void Transcript::putAll(int threadN)
{
	int n = size();
	for(int ii = 0; ii < n; ++ii)
	{
		put(ii, threadN);
	}
}

void Transcript::setAll()
{
	int n = size();
	for(int ii = 0; ii < n; ++ii)
	{
		put(ii, (*this)[ii]->dialogue());
	}
}

int Transcript::openThread(int ti)
{
//	cerr<<"Opening thread "<<ti<<"...\n";
	assert(!_threadsLocked);
	ifstream nstream("/dev/null");
	string name("thread"+intToString(ti));
	_threads[ti] = new Document(nstream, name);
	if(ti >= _nextInd)
	{
		_nextInd = ti + 1;
	}
	return ti;
}

void Transcript::closeThread(int ti)
{
//	cerr<<"Closing thread "<<ti<<"\n";
	assert(_threads[ti]->empty());
	delete _threads[ti];
	_threads.erase(ti);
}

int Transcript::openThread()
{
	return openThread(_nextInd);
}

int Transcript::put(int sent, int threadN)
{
	Sent* toPut = (*this)[sent];
	toPut->setDialogue(threadN);
	int index = toPut->index();

	if(!inMap(_threads, threadN))
	{
		openThread(threadN);
	}

	Document& thread = *_threads[threadN];
	symToNPs& entities = thread.byHead();
	foreach(NPs, np, toPut->nps())
	{
		entities[(*np)->normHeadSym()].push_back(*np);
	}

	int where = 0;
	for(Document::iterator pos = thread.begin();
		pos != thread.end();
		++pos,++where)
	{
		if((*pos)->index() > index)
		{
			thread.insert(pos, toPut);
			return where;
		}
	}

	assert(where == thread.size());
	thread.insert(thread.end(), toPut);
	return where;
}

int Transcript::take(int sent)
{
	Sent* toTake = (*this)[sent];
	int takeFrom = toTake->dialogue();
	toTake->setDialogue(-1);

	Document& thread = *_threads[takeFrom];

	if(!_threadsLocked && thread.size() == 1)
	{
		assert(thread[0] == toTake);
		thread.clear();
		closeThread(takeFrom);
		return 0;
	}

	symToNPs& entities = thread.byHead();
	foreach(NPs, np, toTake->nps())
	{
		NPs& searchThrough = entities[(*np)->normHeadSym()];
		int sz = searchThrough.size();
		for(int rem = sz - 1;
			rem >= 0;
			--rem)
		{
			if(searchThrough[rem] == *np)
			{
				searchThrough.erase(searchThrough.begin() + rem);
				break;
			}
		}
	}

//	cerr<<"\nTaking "<<*toTake<<" num "<<sent<<"\n";
//	cerr<<"target:\n"<<thread<<"\n\n";

	int low = 0;
	int high = thread.size();
	int curr = (high - low) / 2;
	int takeIndex = toTake->index();
	while(true)
	{
//		cerr<<"probing "<<curr<<" time "<<thread[curr]->time()<<"\n";
		if(thread[curr] == toTake)
		{
			break;
		}
		else if(high <= low)
		{
			cerr<<"Didn't find sentence "<<sent<<"\n";
			assert(0);
		}
		else if(thread[curr]->index() < takeIndex)
		{
			low = curr;
			curr = low + (high - low) / 2;
		}
		else if(thread[curr]->index() == takeIndex)
		{
			cerr<<"Can't happen; indices are unique\n";
			assert(0);
		}
		else
		{
			high = curr;
			curr = low + (high - low) / 2;
		}
	}

//	cerr<<"found pos "<<curr<<" erasing\n";
	thread.erase(thread.begin() + curr);
	return curr;
}

Document& Transcript::thread(int thr)
{
	return *_threads[thr];
}

intDocumentMap& Transcript::threads()
{
	return _threads;
}

int Transcript::move(int sent, int targ)
{
//	cerr<<"Moving "<<sent<<" to "<<targ<<"\n";
//	cerr<<"Sent to move is "<<*(*this)[sent]<<"\n";
	take(sent);
	return put(sent, targ);

// 	cerr<<"000000000000000000000000000000\n"<<thread(0)<<"\n";
// 	cerr<<"111111111111111111111111111111\n"<<thread(1)<<"\n";
// 	cerr<<"\n";
}

int Transcript::swap(int sent)
{
	assert(nThreads() == 2);
	int targ = 1 - (*this)[sent]->dialogue();
	return move(sent, targ);
}

int Transcript::nThreads()
{
	return _threads.size();
}

void Transcript::setLocked(bool lock)
{
	_threadsLocked = lock;
}
