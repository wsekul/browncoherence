class LDA;

#ifndef LDA_H
#define LDA_H

#include "Vocab.h"

typedef std::tr1::unordered_map<int, Probs> intProbsMap;

class LDA
{
public:
	LDA(Vocab& voc, string& filePath, strings& docNames);

	LDA(istream&);

	void write(ostream&);
	void read(istream&);

	int topics();

	void projectWord(string word, Probs& topics);

	void readData();

	Vocab _vocab;
	strIntMap _docToGamma;
	string _filePath;
	intProbsMap _beta;
	intProbsMap _gamma;
};

#endif
