class Vocab;

#include "common.h"

#ifndef VOCAB_H
#define VOCAB_H

typedef std::map<int, Vocab> intToVocab;

class Vocab
{
public:
	Vocab();
	Vocab(Vocab& other);

	void clear();

	void write(ostream& os);
	void read(istream& is);

	int size();

	int get(int sym, bool add);
	int get(const string& str, bool add);
	string& inv(int vi);

	void invert();

	void translate(Vocab& other, intIntMap& res);
protected:
	strIntMap _vocab;
	symIntMap _directVocab;
	intToStr _invVocab;
	bool _invClean;
};

class IntVocab
{
public:
	IntVocab();
	IntVocab(IntVocab& other);

	void clear();

	void write(ostream& os);
	void read(istream& is);

	int size();

	int get(unsigned int sym, bool add);
	unsigned int inv(int vi);

	void invert();

	void translate(IntVocab& other, intIntMap& res);
protected:
	symIntMap _directVocab;
	intToSym _invVocab;
	bool _invClean;
};

#endif
