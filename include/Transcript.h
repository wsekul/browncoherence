#include "Document.h"

class Transcript;

#ifndef TRANSCRIPT_H
#define TRANSCRIPT_H

class Transcript : public Document
{
public:
	Transcript(string& name);
	Transcript(char* name);
	Transcript(istream& is, string& name);
	~Transcript();

	void setLocked(bool locked);

	int put(int sent, int thread);
	void putAll(int thread);
	void setAll();

	int take(int sent);

	int move(int sent, int target);
	int swap(int sent);

	int openThread();
	int openThread(int ti);
	void closeThread(int ti);

	Document& thread(int thr);
	intDocumentMap& threads();
	int nThreads();

protected:
	bool _threadsLocked;
	int _nextInd;
	intDocumentMap _threads;
};

#endif
