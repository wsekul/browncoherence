class IBM;

#ifndef IBM_H
#define IBM_H

#include "Vocab.h"
#include "MaxEntSelection.h"
#include "CoherenceModel.h"

struct ContextWord;
struct NounContext;
typedef std::vector<ContextWord> ContextWords;
typedef std::vector<NounContext> NounContexts;

typedef std::map<Tree*, int> TreeToType;
typedef std::map<Tree*, ContextWord> TreeToContext;
typedef std::map<NounContext, ContextWord> ProdToContext;

//NONE must be last, marks special items such as nulls
enum {TR_WORD, TR_NONE};

class IBM : public CoherenceModel
{
public:
	IBM(double smooth, int nulls);
	IBM(istream& in);
	IBM(IBM& other);

	virtual ~IBM();

	virtual void initCaches(Document& doc);
	virtual void clearCaches();

	//two serialization methods: should be inverses of each other

	//serializes this class
	virtual void write(ostream& os);
	//initializes this class -- called by the istream constructor
	virtual void read(istream& is);

	virtual int history();

	virtual void print(ostream& os);
	virtual void projectOntoTopics(Document& doc, Probs& res);
	virtual void nullContext(NounContext*&, MaxEntContext*&);
	virtual Prob projectWord(int word, NounContext* nulls, 
							 MaxEntContext* allNull, Probs& res);

	//writes the expected counts
	virtual void writeCounts(ostream& os);
	//reads in expected counts
	virtual void readCounts(istream& is, intIntMap& vocabTrans);

	virtual void setEstimating(bool status);

	virtual void estimate();
	virtual Prob review(Document& doc);

	virtual ProdToContext& maxAlignments();

	//log probability of a permuted document
	//or does training
	virtual Prob permProbability(Document& doc, ints& perm, bool train);
	virtual Prob emTransfer(Document& doc, ints& perm, bool train,
		IBM* other);

	virtual void normalize();
	virtual void smooth();

	virtual Prob wordProbability(int word, NounContexts& globalContext,
								 NounContext& localContext, bool train);

	virtual Prob wordProbability(int word, NounContexts& globalContext,
								 NounContext& localContext, bool train,
								 ContextWord*& maxAlign);
	virtual Prob emTransferWordProb(int word, 
									NounContexts& globalContext,
									NounContext& localContext, 
									bool train,
									ContextWord*& maxAlign,
									IBM* other);

	static string roleToString(int role);

	static IBM* create(string flag);
	static IBM* create(string flag, istream& is);
	static IBM* create(string flag, IBM* other);

//protected:
	//gets available alignments in prev sentences
	virtual void getContextWords(Sent* sent, NounContext& res, bool add);
	//gets words that need to be aligned in the next sentence
	virtual void getProduced(Sent* sent, 
							 NounContexts& produced, bool train);

	virtual void addNulls(NounContext& prev, string docName);

	virtual Prob getRandBias(int null, int word);
	virtual Prob getRandBiasHelper(int null, int word,
								   intIntProbMap& transCounts,
								   intProbMap& transTotals);

	virtual Prob wordGivenPrevWord(NounContext& nextWord, 
								   ContextWord& prevWord, 
								   bool train, Prob totAlign,
								   MaxEntContext& context);

	virtual Prob transProb(NounContext& next, ContextWord& prev);
	virtual Prob transProbHelper(int next, int prev,
								 intIntProbMap* counts, intProbMap* totals,
								 intProbMap& added);

	virtual void addPartialCounts(NounContext& nextWord, ContextWord& prevWord,
								  MaxEntContext& alignContext, Prob count,
								  Prob totalProb, ContextWord* max);

	virtual void smoothVB(int word, intProbMap& counts, intProbMap* totals,
						  intProbMap& addedFactor, Prob prior);

	virtual void featurizeDistances(NounContext& local, 
									NounContexts& global, bool add);
	virtual int bucket(int d);

	int _prevSents;
	bool _estimating;
	Prob _smooth;
	int _nulls;

	intSet _produced;

	intIntProbMap* _transCounts;
	intIntProbMap* _newTransCounts;
	intProbMap _addedFactor;

	intProbMap* _transTotals;
	intProbMap* _newTransTotals;

	intIntProbMap _counts1;
	intIntProbMap _counts2;
	intProbMap _totals1;
	intProbMap _totals2;

	Vocab _vocab;
	Vocab _alignmentFeatures;
	MaxEntSelection _alignment;

	int VERBOSE;
	ProdToContext _maxAlignments;
};

struct ContextWord
{
	ContextWord():
		word(-1),
		wordInd(-1),
		tree(NULL),
		np(NULL),
		mode(0),
		distFeats(-1),
		pCoref(-1)
	{}

	ContextWord(int wd, Tree* tr):
		word(wd),
		wordInd(-1),
		tree(tr),
		np(NULL),
		mode(0),
		distFeats(-1),
		pCoref(-1)
	{
//		feats[0] = 1;
		feats.push_back(Feat(0, 1));

		if(tr)
		{
			wordInd = Sent::wordsBefore(tr);
		}
	}

	ContextWord(const ContextWord& other):
		word(other.word),
		index(other.index),
		wordInd(other.wordInd),
		tree(other.tree),
		np(other.np),
		mode(other.mode),
		feats(other.feats),
		distFeats(other.distFeats),
		pCoref(other.pCoref)
	{}

	Feats& alignFeats()
	{
		return feats;
	}

	void clearDistFeats()
	{
// 		foreach(ints, df, distFeats)
// 		{
// 			feats.erase(*df);
// 		}
//		distFeats.clear();
		if(distFeats > -1)
		{
			feats.resize(distFeats);
			distFeats = -1;
		}
	}

	void addDistFeat(int df)
	{
		if(distFeats == -1)
		{
			distFeats = feats.size();
		}

//		feats[df] = 1;
		feats.push_back(Feat(df, 1));
//		distFeats.push_back(df);
	}

	void addDistFeat(int df, Prob val)
	{
		if(distFeats == -1)
		{
			distFeats = feats.size();
		}

//		feats[df] = 1;
		feats.push_back(Feat(df, val));
//		distFeats.push_back(df);
	}

	int word;
	int index; //where it appears in the context vector
	int wordInd;
	Tree* tree;
	NP* np;
	int overrideSent;
	int mode;
	Feats feats;
	int distFeats;
	Prob pCoref;
};

struct NounContext : public ContextWords
{
	NounContext():
		word(0),
		tree(NULL),
		sent(0),
		np(NULL),
		pCoref(-1)
	{
		feats.push_back(Feat(0, 1));
	}

	NounContext(int wd, Tree* tr):
		word(wd),
		tree(tr),
		sent(0),
		np(NULL),
		pCoref(-1)
	{
		feats.push_back(Feat(0, 1));
	}

	void addToContext(MaxEntContext& ct)
	{
		foreach(NounContext, word, *this)
		{
			int ind = ct.size();
			ct.push_back(word->alignFeats());
			word->index = ind;
		}
	}

	void clear()
	{
		word = 0;
		tree = NULL;
		ContextWords::clear();
		feats.clear();
		feats.push_back(Feat(0, 1));
		pCoref = -1;
	}

	int word;
	Tree* tree;
	int sent;
	NP* np;
	Feats feats;
	Prob pCoref;
};

bool operator<(const NounContext& p1, const NounContext& p2);

#endif
