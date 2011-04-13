#include "common.h"

class NP;
typedef std::vector<NP*> NPs;
typedef std::set<NP*> NPSet;

typedef std::map<NP*, Prob> NPToProb;
typedef std::map<NP*, NPToProb> NPToNPToProb;

typedef std::map<NP*, int> NPToInt;

typedef std::map<Tree*, NP*> TreeToNP;

#ifndef NP_H
#define NP_H

#include "Sent.h"

//disc-old: refers to a previous NP
//disc-init: first in its coreferential chain
//disc-single: no coreferents in this document
//--disc-new = disc-init U disc-single
enum {DISC_OLD, DISC_INIT, DISC_SINGLE};

//roles for entity grids
//S: subject
//O: object
//X: oblique (something else)
//NONE: doesn't appear

enum {T_SUBJ, T_OBJ, T_X, T_NONE, T_VERB, T_START};
//named entity types detected by opennlp
enum {NE_NONE, NE_PERS, NE_LOC, NE_ORG, NE_PCT, NE_TIME, NE_DATE, NE_MNY};

//Describes a noun phrase.
class NP
{
public:
	NP(Sent* sent, Tree* node, int index, bool discNewMarkable);
	
	Sent* parent();
	int index();
	string fullName();

	string head();
	stIndex headSym();

	//experimental version used a wordnet stemmer and smashed case
	//the tree-reader now smashes case (!?)
	//and the stemmer may be unneeded
	string normHead();
	stIndex normHeadSym();

	bool markable();
	bool discNewMarkable();
	int ref(); 	//referent, or -1
	NP* prev();
	int first();
	int role();
	NP* appositive();
	NP* appositiveTo();

	void setRef(int ref);
	void setPrev(NP* prev);
	void setMarkable(bool mark);
	void setStatus(int stat);
	bool setAppositive(NP* app);

	Tree* node();

	void print(ostream& os);

	//some syntactic analysis
	bool pronoun();
	bool proper();
	bool premodifier();

	int namedEntityType();

	//looks for 'the'
	bool definite();

	//looks for 'a/an' or no determiner
	bool indefinite();

	//detects 'the fact that X'
	bool hasPropositionArg();

	//converts the T_SUBJ &c to printables
	static string roleToString(int role);
	static string discToString(int disc);
	static string neToString(int ne);

protected:
	void findRole();
	void setRef();

	Sent* _parent;
	Tree* _node;
	int _index;
	bool _discNewMarkable;
	bool _markable;
	int _first;
	int _role;
	int _refnum;
	NP* _prevLink;
	NP* _appositive;
	NP* _appositiveTo;
	stIndex _normHeadSym;
};

ostream& operator <<(ostream& os, NP&);

#endif
