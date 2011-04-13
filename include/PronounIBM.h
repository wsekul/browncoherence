class PronounIBM;

#ifndef PRONOUN_IBM_H
#define PRONOUN_IBM_H

#include "ECIBM.h"

enum {NO_MODS, HAS_MODS, PP_MODS, PP_OF_MODS, QUANT_MODS, PHRASAL_MODS,
	  PROPER_MODS, REPEAT_MODS};
enum {NO_TREE, NO_NP, NO_DET, DT_POSSESSIVE, DT_DEF, DT_INDEF,
	  DT_DEICTIC, DT_OTHER};

class PronounIBM : public ECIBM
{
public:
	PronounIBM(int prevSents);
	PronounIBM(istream& in);
	PronounIBM(IBM& other);

	virtual int history();

	virtual void write(ostream& os);
	virtual void read(istream& is);

	virtual void print(ostream& os);

	virtual void initCaches(Document& doc);
	virtual void clearCaches();

	virtual void normalize();

//protected:
	virtual void featurize(ContextWord& created, bool add);
	virtual void featurizeDistances(NounContext& local, 
									NounContexts& global, bool add);

	virtual void distFeatsPostprocess(ContextWord& word, ints& feats);

	virtual Prob wordGivenPrevWord(NounContext& next, ContextWord& prevWord, 
								   bool train,
								   Prob totAlign, MaxEntContext& context);

	virtual void addPartialCounts(NounContext& nextWord, ContextWord& prevWord,
								  MaxEntContext& alignContext, Prob count,
								  Prob total,
								  ContextWord* max);

	virtual void addCombinedFeats(ContextWord& word, ints& feats, bool add);

	static int determinerType(Tree* tr);
	static int modType(Tree* tr, Tree* prev);
	static void modTypes(Tree* tr, Tree* prev, ints& res);
	static int govType(Tree* tr);
	static void collectMods(Tree* tr, Trees& mods);

	virtual Prob emTransferWordProb(int nextWord,
									NounContexts& global,
									NounContext& local, bool train,
									ContextWord*& max,
									IBM* otherModel);

	virtual void readLexicalStatistics(istream& is);

	static string modsToString(int mod);
	static string detToString(int det);

	intToProb _animacy;
	intToProb _frequency;

	intToInt _occurrences;
	strSet _nonAna;
};

#endif
