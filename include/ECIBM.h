class ECIBM;

#ifndef EC_IBM_H
#define EC_IBM_H

#include "TopicIBM.h"
#include "Features.h"

enum {N_SING, N_PLUR};
enum {FIRST, SECOND, THIRD};
enum {GEN_MASC, GEN_FEM, GEN_NEUT, GEN_UNKNOWN};

const int NUMSENTTYP = 3;
const int NUMSYNTAXTYP = 3;
const int NUMANTEWTYP = 6;
const int NUMPROWTYP = 4;
const int NUMPROTYP = 3;
const int NUMANTETYP = 4;
const int ANTEDISTSZ = (NUMSENTTYP*NUMSYNTAXTYP*NUMANTEWTYP*
						NUMPROWTYP*NUMPROTYP*NUMANTETYP);
const int ANTESENTSEP = (NUMSYNTAXTYP*NUMANTEWTYP*NUMPROWTYP*
						 NUMPROTYP*NUMANTETYP);
const int SYNTAXNOSEP = (NUMANTEWTYP*NUMPROWTYP*NUMPROTYP*NUMANTETYP);
const int ANTEWORDSEP = (NUMPROWTYP*NUMPROTYP*NUMANTETYP);
const int PROTYPSEP = (NUMPROTYP*NUMANTETYP);
const int ANTETYPSEP = (NUMANTETYP);

class ECIBM : public TopicIBM
{
public:
	ECIBM();
	ECIBM(istream& in);
	ECIBM(IBM& other);

	virtual void write(ostream& os);
	virtual void read(istream& is);

	virtual void initCaches(Document& doc);
	virtual void clearCaches();

//protected:
	//gets words that need to be aligned in the next sentence
	virtual void getProduced(Sent* sent, 
							 NounContexts& produced, bool train);

	virtual void getContextWords(Sent* sent, 
								 NounContext& produced, bool train);

	virtual void addNulls(NounContext& prev, string docName);
	virtual void featurize(ContextWord& created, bool add);

	virtual Prob wordGivenPrevWord(NounContext& next, ContextWord& prevWord, 
								   bool train,
								   Prob totAlign, MaxEntContext& context);

	virtual void addPartialCounts(NounContext& nextWord, ContextWord& prevWord,
								  MaxEntContext& alignContext, Prob count,
								  Prob total,
								  ContextWord* max);

	virtual bool constraintProhibits(NounContext& next, ContextWord& prevWord);

	virtual int person(Tree* word);
	virtual int gender(Tree* word);
	virtual bool isQuoted(NP& np);
	virtual int bucketDist(int dist);
	virtual int bucketSent(int dist);
	virtual int anteType(Tree* word);
	virtual int proType(Tree* word);
	virtual int anteTabIndex(NounContext& next, ContextWord& prev, ints& feats);

	virtual intProbMap* getGenders(Tree* tree);

	TreeSet _quotedWords;

	//this is a huge blob of numbers with little internal structure
	//the indexing logic is in a function somewhere
	Probs _pAnte;

	//number: EC's system knows 2 numbers, sing/plur (no 'other' class)
	intProbMap _pSingGivenNum;

	//3 persons, pro in quotes, ante in quotes
	//mapping is pers -> pers -> ante/pro -> prob
	intIntIntProbMap _pPers;

	//gender: 3 genders
	//mapping is POS -> word -> gen -> prob
	intIntIntProbMap _pGen;

	strSet INDEF;
};

#endif
