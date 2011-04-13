#ifndef COMMON_H
#define COMMON_H

//includes
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <list>
#include <vector>
#include <map>
#include <set>

#include <string.h>
#include <stdlib.h>

#include <assert.h>

#include <tr1/unordered_map>
#include <tr1/unordered_set>

#include <algorithm>
#include <math.h>

#include "Tree.h"
#include "SymbolTable.h"
#include "ntInfo.h"
#include "setLabs.h"
#include "headFinder.h"

#include <popen.h>

#ifdef USE_WORDNET
#include "wn.h"
#endif

#include "gsl_rng.h"
#include "gsl_randist.h"

using std::cin;
using std::cout;
using std::cerr;

using std::string;

typedef double Prob;

//lots of types
typedef std::set<Tree*> TreeSet;
typedef std::vector<Tree*> Trees;
typedef std::map<Tree*, int> TreeToInt;

typedef std::set<int> intSet;
typedef std::set<intSet> intSets;
typedef std::list<intSet> intSetLst;
typedef std::vector<int> ints;
typedef std::list<int> intLst;
typedef std::list<ints> intsLst;
typedef std::set<ints> intsSet;

typedef std::set<double> doubleSet;
typedef std::vector<double> doubles;

typedef std::set<stIndex> symSet;
typedef std::vector<stIndex> syms;

typedef std::set<string> strSet;
typedef std::set<strSet> strSets;
typedef std::list<string> StringList;
typedef std::vector<string> strings;

typedef std::vector<Prob> Probs;
typedef std::vector<Probs> ProbMat;
typedef std::vector<ProbMat> ProbMat3;

typedef std::map<int, int> intToInt;
typedef std::map<int, Prob> intToProb;
typedef std::map<int, double> intToDouble;
typedef std::map<int, stIndex> intToSym;
typedef std::map<int, string> intToStr;

typedef std::map<double, int> doubleToInt;

typedef std::map<stIndex, int> symToInt;

typedef std::map<int, intToProb> intToIntToProb;
typedef std::map<int, intToInt> intToIntToInt;
typedef std::map<stIndex, intToInt> symToIntToInt;

typedef std::map<int, intToIntToProb> intToIntToIntToProb;

typedef std::map<string, string> strToStr;

typedef std::map<string, int> strToInt;
typedef std::map<string, strToInt> strToStrToInt;

typedef std::map<string, double> strToDouble;
typedef std::map<string, strToDouble> strToStrToDouble;

typedef std::map<string, Prob> strToProb;
typedef std::map<string, strToProb> strToStrToProb;

typedef std::set<string> strSet;

typedef std::tr1::unordered_map<stIndex, int> symIntMap;

typedef std::tr1::unordered_map<int, int> intIntMap;
typedef std::tr1::unordered_map<int, intIntMap> intIntIntMap;
typedef std::tr1::unordered_map<int, Prob> intProbMap;
typedef std::tr1::unordered_map<int, intProbMap> intIntProbMap;
typedef std::tr1::unordered_map<int, intIntProbMap> intIntIntProbMap;
typedef std::tr1::unordered_map<int, Probs> intProbsMap;

typedef std::tr1::unordered_map<string, string> strStrMap;
typedef std::tr1::unordered_map<int, strStrMap> intStrStrMap;
typedef std::tr1::unordered_map<string, Prob> strProbMap;
typedef std::tr1::unordered_map<string, int> strIntMap;
typedef std::tr1::unordered_map<string, intProbMap> strIntProbMap;

typedef std::tr1::unordered_map<string, strProbMap> strStrProbMap;

//MJ's loop macros
// foreach is a simple loop construct
//
//   STORE should be an STL container
//   TYPE is the typename of STORE
//   VAR will be defined as a local variable of type TYPE::iterator
//
#define foreach(TYPE, VAR, STORE) \
   for (TYPE::iterator VAR = (STORE).begin(); VAR != (STORE).end(); ++VAR)

// cforeach is just like foreach, except that VAR is a const_iterator
//
//   STORE should be an STL container
//   TYPE is the typename of STORE
//   VAR will be defined as a local variable of type TYPE::const_iterator
//
#define cforeach(TYPE, VAR, STORE) \
   for (TYPE::const_iterator VAR = (STORE).begin(); VAR != (STORE).end(); ++VAR)

// revforeach is like foreach, except backwards
#define revforeach(TYPE, VAR, STORE) \
   for (TYPE::reverse_iterator VAR = (STORE).rbegin(); VAR != (STORE).rend(); ++VAR)

//utilities
extern SymbolTable* SYMTAB;
extern gsl_rng* RNG;
extern symSet MONTHS;

const string DATA_PATH = "./data";

//the ONE TRUE initializer!
//all applications should call this!
void appInit(const string& data);

const string UNKNOWN = "_UNKNOWN";

//downcase/upcase a string
string lc(const string& str);
string uc(const string& str);

string intToString(int x);

bool endswith(const string& check, const string& suffix);

extern intStrStrMap STEM_CACHE;
string& stem(const string& str);
string& stem(Tree* tree);

//read back a check token to verify loader is working
void checkToken(istream& is, const string& str);

//get a random permutation of indices 0..n
bool isIdentity(ints& perm);
void randPermutation(ints& iperm, int size);

//backward compat version
string oldStem(const string& str, int pos);
int posToInt(const string& type);

bool isNum(const string& word);
bool isAlphaNum(const string& word);

inline bool isProb(Prob p)
{
	return(p <= 1 && p >= 0 && isfinite(p));
}

inline Prob smoothExtremes(Prob p)
{
	if(p > 1 - 1e-10)
	{
		return 1 - 1e-10;
	}
	else if(p < 1e-10)
	{
		return 1e-10;
	}
	else
	{
		return p;
	}
}

#define assertProb(p) assert((p) <= 1.01 && (p) >= 0 && isfinite((p)))

#define assertLogProb(p) assert((p) <= 0 && isfinite((p)))

#define assertEQ(p, q) assert(fabs((p) - (q)) < 1e-5)

//set containment
template <class key>
inline bool contains(std::set<key>& st, key item)
{
	return st.count(item) > 0;
}

template <class key, class val>
inline bool inMap(std::map<key, val>& mm, key item)
{
	typename std::map<key, val>::iterator entry = mm.find(item);
	return (entry != mm.end());
}

template <class key, class val>
inline bool inMap(std::tr1::unordered_map<key, val>& mm, key item)
{
	typename std::tr1::unordered_map<key, val>::iterator entry = mm.find(item);
	return (entry != mm.end());
}

template <class key, class val>
inline val ifExists(std::map<key, val>& mm, key item, val deflt)
{
	typename std::map<key, val>::iterator entry = mm.find(item);
	return (entry == mm.end()) ? deflt : entry->second;
}

template <class key, class val>
inline val ifExists(std::tr1::unordered_map<key, val>& mm, key item, 
					val deflt)
{
	typename std::tr1::unordered_map<key, val>::iterator entry = mm.find(item);
	return (entry == mm.end()) ? deflt : entry->second;
}

//sorting stuff
struct PriNode
{
	PriNode();
	PriNode(double p, string s):pri(p),data(s){}
	double pri;
	string data;
};

struct IntPriNode
{
	IntPriNode();
	IntPriNode(double p, int s):pri(p),data(s){}
	double pri;
	int data;
};

class Greater :
        public std::binary_function<const PriNode, const PriNode,
        bool>
{
public:
	bool operator()(const PriNode& p1, const PriNode& p2)
	{
		return p1.pri > p2.pri;
	}
};

class IntGreater :
        public std::binary_function<const IntPriNode, const IntPriNode,
        bool>
{
public:
        bool operator()(const IntPriNode& p1, const IntPriNode& p2)
	{
		return p1.pri > p2.pri;
	}
};

typedef std::vector<PriNode> PriVec;
typedef std::vector<IntPriNode> IntPriVec;

//map writes/reads
template <class key, class key2, class key3, class val>
void writeMap3(std::map<key, std::map<key2, 
			   std::map<key3, val> > >& m, ostream& os)
{
	typename std::map<key, std::map<key2, std::map<key3, val> > >::iterator i;
	for(i = m.begin();
		i != m.end();
		i++)
	{
		os<<(*i).first<<"\n";

		writeMap2((*i).second, os);
	}

	os<<">>>>\n";
}

template <class key, class key2, class val>
void writeMap2(std::map<key, std::map<key2, val> >& m, ostream& os)
{
	typename std::map<key, std::map<key2, val> >::iterator i;
	for(i = m.begin();
		i != m.end();
		i++)
	{
		os<<(*i).first<<"\n";

		writeMap((*i).second, os);
	}

	os<<">>>\n";
}

template <class key, class key2, class key3, class val>
void readMap3(std::map<key, std::map<key2, 
			  std::map<key3, val> > >& m, istream& is)
{
	string keyStr;
	
	while(is>>keyStr)
	{
		if(keyStr == ">>>>")
		{
			break;
		}

		std::istringstream convert;
		convert.str(keyStr);
		key kObj;
		convert>>kObj;

		std::map<key2, std::map<key3, val> > submap;
		typename std::map<key, std::map<key2, 
			std::map<key3, val> > >::iterator keyEntry = m.find(kObj);
		
		if(keyEntry != m.end())
		{
			//not needed, strictly speaking, since maps can only
			//have unique keys -- but some of my scripts write this
			//format as well
			submap = (*keyEntry).second;
		}

		readMap2(submap, is);

		m[kObj] = submap;
	}
}

template <class key, class key2, class val>
void readMap2(std::map<key, std::map<key2, val> >& m, istream& is)
{
	string keyStr;
	
	while(is>>keyStr)
	{
		if(keyStr == ">>>")
		{
			break;
		}

		std::istringstream convert;
		convert.str(keyStr);
		key kObj;
		convert>>kObj;

		std::map<key2, val> submap;
		typename std::map<key, std::map<key2, val> >::iterator keyEntry =
			m.find(kObj);
		
		if(keyEntry != m.end())
		{
			//not needed, strictly speaking, since maps can only
			//have unique keys -- but some of my scripts write this
			//format as well
			submap = (*keyEntry).second;
		}

		readMap(submap, is);

		m[kObj] = submap;
	}
}

template <class key, class val>
void writeMap(std::map<key, val>& m, ostream& os)
{
 	typename std::map<key, val>::iterator j;
	for(j = m.begin();
		j != m.end();
		j++)
	{
		os<<"\t"<<(*j).first<<"\t"<<(*j).second<<"\n";
	}

	os<<"\n>>\n";
}

template <class key, class val>
void readMap(std::map<key, val>& m, istream& is)
{
	string k;
	string v;

	while(is>>k)
	{
		if(k == ">>")
		{
			break;
		}

		std::istringstream convert;
		convert.str(k);
		key kObj;
		convert>>kObj;
		
		is>>v;

		std::istringstream convert2;
		convert2.str(v);
		val vObj;
		convert2>>vObj;

		m[kObj] = vObj;
	}
}

//map writes/reads
template <class key, class key2, class key3, class val>
void writeHashMap3(std::tr1::unordered_map<key, std::tr1::unordered_map<key2, 
				   std::tr1::unordered_map<key3, val> > >& m, ostream& os)
{
	typename std::tr1::unordered_map<key,
		std::tr1::unordered_map<key2, 
		std::tr1::unordered_map<key3, val> > >::iterator i;
	for(i = m.begin();
		i != m.end();
		i++)
	{
		os<<(*i).first<<"\n";

		writeHashMap2((*i).second, os);
	}

	os<<">>>>\n";
}

template <class key, class key2, class val>
void writeHashMap2(std::tr1::unordered_map<key, 
				   std::tr1::unordered_map<key2, val> >& m, ostream& os)
{
	typename std::tr1::unordered_map<key, 
		std::tr1::unordered_map<key2, val> >::iterator i;
	for(i = m.begin();
		i != m.end();
		i++)
	{
		os<<(*i).first<<"\n";

		writeHashMap((*i).second, os);
	}

	os<<">>>\n";
}

template <class key, class key2, class key3, class val>
void readHashMap3(std::tr1::unordered_map<key, std::tr1::unordered_map<key2, 
				  std::tr1::unordered_map<key3, val> > >& m, istream& is)
{
	string keyStr;
	
	while(is>>keyStr)
	{
		if(keyStr == ">>>>")
		{
			break;
		}

		std::istringstream convert;
		convert.str(keyStr);
		key kObj;
		convert>>kObj;

		std::tr1::unordered_map<key2, 
			std::tr1::unordered_map<key3, val> > submap;
		typename std::tr1::unordered_map<key, 
			std::tr1::unordered_map<key2, 
			std::tr1::unordered_map<key3, 
			val> > >::iterator keyEntry = m.find(kObj);
		
		if(keyEntry != m.end())
		{
			//not needed, strictly speaking, since maps can only
			//have unique keys -- but some of my scripts write this
			//format as well
			submap = (*keyEntry).second;
		}

		readHashMap2(submap, is);

		m[kObj] = submap;
	}
}

template <class key, class key2, class val>
void readHashMap2(std::tr1::unordered_map<key, 
				  std::tr1::unordered_map<key2, val> >& m, istream& is)
{
	string keyStr;
	
	while(is>>keyStr)
	{
		if(keyStr == ">>>")
		{
			break;
		}

		std::istringstream convert;
		convert.str(keyStr);
		key kObj;
		convert>>kObj;

		std::tr1::unordered_map<key2, val> submap;
		typename std::tr1::unordered_map<key, 
			std::tr1::unordered_map<key2, val> >::iterator keyEntry =
			m.find(kObj);
		
		if(keyEntry != m.end())
		{
			//not needed, strictly speaking, since maps can only
			//have unique keys -- but some of my scripts write this
			//format as well
			submap = (*keyEntry).second;
		}

		readHashMap(submap, is);

		m[kObj] = submap;
	}
}

template <class key, class val>
void writeHashMap(std::tr1::unordered_map<key, val>& m, ostream& os)
{
 	typename std::tr1::unordered_map<key, val>::iterator j;
	for(j = m.begin();
		j != m.end();
		j++)
	{
		os<<"\t"<<(*j).first<<"\t"<<(*j).second<<"\n";
	}

	os<<"\n>>\n";
}

template <class key, class val>
void readHashMap(std::tr1::unordered_map<key, val>& m, istream& is)
{
	string k;
	string v;

	while(is>>k)
	{
		if(k == ">>")
		{
			break;
		}

		std::istringstream convert;
		convert.str(k);
		key kObj;
		convert>>kObj;
		
		is>>v;

		std::istringstream convert2;
		convert2.str(v);
		val vObj;
		convert2>>vObj;

		m[kObj] = vObj;
	}
}
//end hashreads

template <class key>
void writeSet(std::set<key>& s, ostream& os)
{
 	typename std::set<key>::iterator j;
	for(j = s.begin();
		j != s.end();
		j++)
	{
		os<<"\t"<<*j<<"\n";
	}

	os<<"\n>>\n";
}

template <class key>
void readSet(std::set<key>& s, istream& is)
{
	string k;

	while(is>>k)
	{
		if(k == ">>")
		{
			break;
		}

		std::istringstream convert;
		convert.str(k);
		key kObj;
		convert>>kObj;
		
		s.insert(kObj);
	}
}

template <class key>
void writeVector(std::vector<key>& v, ostream& os)
{
	int dim = v.size();
	os<<dim<<"\t";
	for(int i=0; i < dim; i++)
	{
		os<<v[i]<<" ";
	}

	os<<"\n";
}

template <class key>
void readVector(std::vector<key>& v, istream& is)
{
	int dim;
	is>>dim;
	v.resize(dim);

	for(int i=0; i<dim; i++)
	{
		is>>v[i];
	}
}

#endif
