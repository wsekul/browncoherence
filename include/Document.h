#include "common.h"

class Document;
typedef std::vector<Document*> Documents;

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "Sent.h"
#include "NP.h"

typedef std::vector<Document*> Documents;
typedef std::tr1::unordered_map<int, Document*> intDocumentMap;

typedef std::map<int, NP*> intToNP;
typedef std::map<int, NPs> intToNPs;
typedef std::map<int, NPSet> intToNPSet;
typedef std::map<stIndex, NPs> symToNPs;

//Holds a document's sentences.
class Document : public std::vector<Sent*>
{
public:
	Document(string& name);
	Document(char* name);
	Document(istream& is, string& name);
	~Document();

	//segments are conversational 'turns' as in the switchboard corpus
	//signaled by (CODE (SYM Speaker??)) trees
	//if you're interested in actually using this functionality, contact me--
	//I have a few other tools and ideas regarding dialogue
	int segments();
	//this list contains the indices of each seg start, and one more index
	//which is one beyond the end of the document
	//(this allows printing of the last segment using the same stopping
	//criterion as the others)
	ints& segmentStarts();

	void print(ostream& os);

	string name();

	NPSet& markables();
	NPSet& nps();
	intToNPSet& byReferent();
	intToNP& byID();
	symToNPs& byHead();

	int entities();

	void readMarkablesKey(istream& is, bool onlyParserMentions);
	void writeMarkablesKey(ostream& os);

	void makeAllMarkable();

	void clearReferenceInfo();
	void clearPredecessorLinks();

protected:
	void read(istream& is);
	NP* findNP(string& word, int num, bool onlyParserMentions);
	void printWithRefs(ostream& os, Tree* tree, TreeToInt& treeToRef);

	string _name;
	ints _segments;
	NPSet _markables;
	NPSet _nps;
	intToNPSet _byReferent;
	intToNP _byID;
	symToNPs _byHead;
};

//pass over the next Document in the input
//returns true if the Document would have been non-empty
bool skipDocument(istream& is);

ostream& operator <<(ostream& os, Document&);

#endif
