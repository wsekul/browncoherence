class EGridModel;

#ifndef E_GRID_MODEL_H
#define E_GRID_MODEL_H

#include "CoherenceModel.h"

//'cause generativity would allow us to compare different documents with
//one another better
//and makes EC happy!
enum { NOT_GEN, SLIGHTLY_GEN, MORE_GEN, REALLY_GEN };

//a history in the entity grid is [s1 ... s_h, salience]
typedef std::list<int> History;
typedef std::vector<History> Histories;

//some entity grid utilities
class EGridModel : public CoherenceModel
{
public:
	EGridModel(int hist, int sal);

	virtual ~EGridModel();

	virtual int history();

	//returns an upper bound on the number of histories that exist
	virtual int numContextTypes();

	//converts a history to a number
	virtual int histToNum(History& hist);
	virtual int histToNum(History& hist, int history);

	//converts a number to a history
	virtual History numToHist(int num);

	//detects histories which can simply never occur, such as
	// [S S salience = 1], or [S <s> ...]
	virtual bool possible(const History& hist);

	//for printing histories
	virtual string histStr(const History& hist);

	//cache info about a document -- this creates the actual entity grid
	// (_roles) and the salience counts (_occurrences)
	virtual void initCaches(Document& doc);
	virtual void clearCaches();

	//get the role cache (the actual egrid)
	virtual symToIntToInt& roles();

//protected:
	virtual void cacheRoles(Document& doc);
	virtual void cacheOccurrences(Document& doc);

	int _history; //history size
	int _maxSalience; //salience levels, up to (and incl.) this number

	//caches
	intToInt _occurrences;
	symToIntToInt _roles; //entity -> sent # -> role
	intToNPs _reps;
};

#endif
