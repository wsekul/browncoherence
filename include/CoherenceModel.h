class CoherenceModel;

#ifndef COHERENCE_MODEL_H
#define COHERENCE_MODEL_H

#include "common.h"
#include "Document.h"

const int GLOBAL_HIST = 10000; //more than length of largest expected document

typedef std::vector<CoherenceModel*> CoherenceModels;

//this is the abstract class that coherence models are expected to inherit
class CoherenceModel
{
public:
	virtual ~CoherenceModel();

	//two serialization methods: should be inverses of each other

	//serializes this class
	virtual void write(ostream& os) = 0;
	//initializes this class -- called by the istream constructor
	virtual void read(istream& is) = 0;

	//you are allowed to print debugging info using this method
	//it doesn't have to do anything
	virtual void print(ostream& os);

	//cache info about a document
	virtual void initCaches(Document& doc) = 0;
	virtual void clearCaches() = 0;

	virtual int history();

	//log probability of a permuted document
	//or does training
	virtual Prob permProbability(Document& doc, ints& perm, bool train) = 0;
	virtual Probs& sentScores();
	virtual Prob globalScore();

	//main useful method of this model
	//you don't have to write it, because it wraps more basic calls
	//does init-cache / perm-prob (identity perm) / clear-cache / return
	virtual Prob logProbability(Document& doc);

	//add a training document
	//does init-cache / perm-prob (ident) true / clear-cache / return
	virtual void train(Document& doc);
	
	//estimate parameters
	virtual void estimate() = 0;

	//the flagset, as a static string
	//the Right Way would be an association between these and the
	//constructors, but that'd mean fn ptrs, which I'm bad at
	static string FLAGS;

	//the master loader... see above re: hackiness of this
	//you have to keep updating the flag list to agree with train
	//but it's way better than having a copy of this code in every
	//driver file
	static CoherenceModel* loadFromFile(char* flag, char* file);
	static CoherenceModel* loadByFlag(char* flag, istream& is);

	//nextArg should point to the first argument to be read
	//and when done, will point to the next argument to be read
	static CoherenceModel* loadFromArgs(char* args[], int argc, int& nextArg);

	Probs _sentScores;
	Prob _globalScore;
};

#endif
