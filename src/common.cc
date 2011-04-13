#include "common.h"

#ifdef USE_TAO
#include "tao-optimizer.h"
#endif

#include "Sent.h"

SymbolTable* SYMTAB = NULL;
gsl_rng* RNG = NULL;
symSet MONTHS;
intStrStrMap STEM_CACHE;

void appInit(const string& data)
{
#ifdef USE_TAO
	int fakeargc = 1;
	char* fakeargv[1];
	fakeargv[0] = "foo";
	char** fakeargs = fakeargv;
	new tao_environment(fakeargc, fakeargs);
#endif

#ifdef USE_WORDNET
	if(wninit())
	{
		cerr<<"Fatal error with wordnet database.\n";
		abort();
	}
#endif

	Tree::singleLineReader = 1;

	//initialize the tree reader
	SYMTAB = new SymbolTable();

	string ntInfoFile = data+"/ntInfo.txt";
	ifstream ntfs(ntInfoFile.c_str());
	if(!ntfs.is_open())
	{
		cerr<<"Can't open "<<ntInfoFile<<"\n";
		abort();
	}

	readNtInfo(ntfs,SYMTAB);

	string headInfoFile = data+"/headInfo.txt";
	ifstream hfs(headInfoFile.c_str());
	if(!hfs.is_open())
	{
		cerr<<"Can't open "<<headInfoFile<<"\n";
		abort();
	}
	readHeadInfo(hfs,SYMTAB);

	RNG = gsl_rng_alloc(gsl_rng_taus);

	setLabs(SYMTAB);
	MONTHS.insert(SYMTAB->toIndex("january"));
	MONTHS.insert(SYMTAB->toIndex("february"));
	MONTHS.insert(SYMTAB->toIndex("january"));
	MONTHS.insert(SYMTAB->toIndex("march"));
	MONTHS.insert(SYMTAB->toIndex("april"));
	MONTHS.insert(SYMTAB->toIndex("may"));
	MONTHS.insert(SYMTAB->toIndex("june"));
	MONTHS.insert(SYMTAB->toIndex("july"));
	MONTHS.insert(SYMTAB->toIndex("august"));
	MONTHS.insert(SYMTAB->toIndex("september"));
	MONTHS.insert(SYMTAB->toIndex("october"));
	MONTHS.insert(SYMTAB->toIndex("november"));
	MONTHS.insert(SYMTAB->toIndex("december"));

	MONTHS.insert(SYMTAB->toIndex("jan."));
	MONTHS.insert(SYMTAB->toIndex("feb."));
	MONTHS.insert(SYMTAB->toIndex("mar."));
	MONTHS.insert(SYMTAB->toIndex("apr."));
	MONTHS.insert(SYMTAB->toIndex("aug."));
	MONTHS.insert(SYMTAB->toIndex("sept."));
	MONTHS.insert(SYMTAB->toIndex("oct."));
	MONTHS.insert(SYMTAB->toIndex("nov."));
	MONTHS.insert(SYMTAB->toIndex("dec."));
}

//downcase/upcase a string
string lc(const string& str)
{
	char* buf = new char[str.length()];
	str.copy(buf, str.length());
	for(int i = 0; i < str.length(); i++)
		buf[i] = tolower(buf[i]);
	string copy(buf, str.length());
	delete[] buf;
	return copy;
}

string uc(const string& str)
{
	char* buf = new char[str.length()];
	str.copy(buf, str.length());
	for(int i = 0; i < str.length(); i++)
		buf[i] = toupper(buf[i]);
	string copy(buf, str.length());
	delete[] buf;
	return copy;
}

bool endswith(const string& check, const string& suffix)
{
	return check.substr(check.size() - suffix.size(), suffix.size()) ==
		suffix;
}

string intToString(int x)
{
	ostringstream cvt;
	cvt<<x;
	return cvt.str();
}

int posToInt(const string& type)
{
#ifdef USE_WORDNET
	if(type[0] == 'N')
	{
		return NOUN;
	}
	else if(type[0] == 'J')
	{
		return ADJ;
	}
	else if(type[0] == 'R')
	{
		return ADV;
	}
	else if(type[0] == 'V' || type == "MD" || type == "AUX")
	{
		return VERB;
	}
	else
	{
		return NOUN; //stemmer won't work, closed-class word
	}
#else
	cerr<<"Warning: this function doesn't work without WORDNET.\n";
	return -1;
#endif
}

string oldStem(const string& str, int tag)
{
#ifdef USE_WORDNET
	char* sense = morphword(const_cast<char*>(str.c_str()), tag);
	if(sense != NULL)
	{
		string copy(sense);
		return copy;
	}
#else
	cerr<<"Warning: this function doesn't work without WORDNET.\n";
#endif
	return str;
}

string& stem(const string& str)
{
#ifdef USE_WORDNET
	strStrMap& nounCache = STEM_CACHE[NOUN];
	strStrMap::iterator entry = nounCache.find(str);
	if(entry != nounCache.end())
	{
		return entry->second;
	}

	char* sense = morphword(const_cast<char*>(str.c_str()), NOUN);
	if(sense != NULL)
	{
		string copy(sense);
		nounCache[str] = copy;
	}
	else
	{
		nounCache[str] = str;
	}

#else
	cerr<<"Warning: this function doesn't work without WORDNET.\n";
	nounCache[str] = str;
#endif

	return nounCache[str];
}

string& stem(Tree* tree)
{
#ifdef USE_WORDNET
	int pos = posToInt(Sent::tag(tree));
	strStrMap& cache = STEM_CACHE[pos];
	string str = Sent::word(tree->subtrees);

	strStrMap::iterator entry = cache.find(str);
	if(entry != cache.end())
	{
		return entry->second;
	}

	char* sense = morphword(const_cast<char*>(str.c_str()), pos);
	if(sense != NULL)
	{
		string copy(sense);
		cache[str] = copy;
	}
	else
	{
		cache[str] = str;
	}

#else
	cerr<<"Warning: this function doesn't work without WORDNET.\n";
	cache[str] = str;
#endif

	return cache[str];
}

bool isNum(const string& word)
{
	for(int i=0; i<word.length(); i++)
	{
		if(!isdigit(word[i]))
		{
			return false;
		}
	}
	return true;
}

bool isAlphaNum(const string& word)
{
	for(int i=0; i<word.length(); i++)
	{
		if(isdigit(word[i]))
		{
			return true;
		}
	}
	return false;
}

void randPermutation(ints& iperm, int size)
{
	iperm.clear();
	for(int i=0; i<size; i++)
	{
		iperm.push_back(i);
	}

	random_shuffle(iperm.begin(), iperm.end()); 	//STL random shuffle
}

/*
void randPermutation(ints& iperm, int size)
{
	//EC's permutation function
	int perm[5000];
	assert(iperm.size()<5000);
	for(int i=0; i<5000; i++)
	{
		perm[i]=-1;
	}

	for(i=0;i<size;i++)
	{
		//put the items [0..size) into the large array
		//by picking a random index and inserting each one
		//this is correct if no index is selected twice (ie size << 5000)
		int pos = rand()%5000;
		for(j=pos;;j--)
		{
			//the loop makes the function work if an index is selected twice
			if(perm[j]<0)
			{
				perm[j]=i;
				break;
			}
			else if(j==0)
			{
				j=5000;
			}
		}
	}

	//put the items into the permutation vector
	iperm.clear();
	for(i=0;i<5000;i++)
	{
		if(perm[i]>=0)
		{
			iperm.push_back(perm[i]);
		}
	}
}
*/

void checkToken(istream& is, const string& str)
{
	string key;
	is>>key;
	if(key != str)
	{
		cerr<<"Expected "<<str<<", got "<<key<<"\n";
		assert(0);
	}
}

bool isIdentity(ints& perm)
{
	int size = perm.size();
	for(int i = 0; i < size; i++)
	{
		if(perm[i] != i)
		{
			return false;
		}
	}
	return true;
}
