class Sent;

#ifndef SENT_H
#define SENT_H

#include "common.h"
#include "Tree.h"

#include "Document.h"
#include "NP.h"

//Wrapper for the tree class, supplies tree annotation routines.
class Sent
{
public:
	Sent();
	Sent(Document& doc, int ind, int speaker, int dial, int time, 
		 intSet& mentioned);
	Sent(Document& doc, int ind, int speaker, int dial, int time, 
		 intSet& mentioned, Tree*);
	Sent(Sent& other);
	~Sent();

	void read(istream& is);

	void print(ostream& os);

	int index();
	Document* parent();
	Tree* tree();
	int speaker();
	int dialogue();
	int time();
	intSet& mentioned();

	void setDialogue(int dial);

	string fullName();
	NPs& nps();

	//Tree utility fns

	//word of this tree, or empty str
	static inline string word(Tree* tree)
	{
		return leaf(tree) ? SYMTAB->toString(tree->label) : "";
	}

	static inline string tag(Tree* tree)
	{
		return leaf(tree) ? "" : SYMTAB->toString(tree->label);
	}

	static inline bool leaf(Tree* tree)
	{
		return tree->subtrees == NULL;
	}

	static inline bool preterm(Tree* tree)
	{
		return terminal_p(tree->label);
	}

	static inline bool sentType(Tree* tree)
	{
		return tree->label == sLabel || tree->label == sbarLabel || 
			tree->label == sinvLabel || tree->label == s1Label;
	}

	static inline bool isVerb(Tree* tree)
	{
		return tree->label == vbnLabel || tree->label == vbgLabel || 
			tree->label == vbdLabel || tree->label == vbzLabel ||
			tree->label == vbLabel || tree->label == vbpLabel;
	}

	static inline bool whType(Tree* tree)
	{
		return tree->label == whnpLabel || tree->label == whppLabel ||
			tree->label == wpLabel || tree->label == wdtLabel ||
			tree->label == wpdLabel || tree->label == wrbLabel ||
			tree->label == whadvpLabel;
	}

	static inline bool proper(stIndex ind)
	{
		return ind == nnpLabel || ind == nnpsLabel;
	}

	static inline bool sameHead(Tree* t1, Tree* t2)
	{
		return head(t1)->subtrees->label == head(t2)->subtrees->label;
	}

	static Tree* headChild(Tree* tree);
	//preterm over the head word
	static inline Tree* head(Tree* tree)
	{
		return tree->htree;
	}

	static inline stIndex headSym(Tree* tree)
	{
		return tree->htree->subtrees->label;
	}
	static inline stIndex headTagSym(Tree* tree)
	{
		return tree->htree->label;
	}
	static string headWord(Tree* tree);
	static string headTag(Tree* tree);

	//all the words in a tree, as a long string
	static string plaintext(Tree* tree);
	static void plaintext(Tree* tree, std::ostringstream& os); //helper

	//all the preterminals-- NOT preloaded but computed on the fly
	static void getLeaves(Tree* tree, Trees& res);

	//all the modifiers
	static void getModifiers(Tree* tree, Trees& leftMods, Trees& rightMods);

	static bool identical(Tree* one, Tree* two);

	//returns the first child of this tree with the specified label
	//or null
	static Tree* hasChild(Tree* node, stIndex label);

	//for finding ancestors

	static bool isAncestor(Tree* dec, Tree* anc);

	//first ancestor with label
	static Tree* ancestor(Tree* node, stIndex label);
	//last ancestor with label
	static Tree* highestAncestor(Tree* node, stIndex label);

	//for navigating out of recursive structures

	//returns the first parent of this tree without the specified label
	//or null
	static Tree* firstParent(Tree* node, stIndex notLabel);
	//returns the last parent of this tree with the specified label
	//or the node itself
	static Tree* lastParent(Tree* node, stIndex label);

	//places into res all the descendants of node with the specified label
	static void collectTrees(Tree* node, stIndex label, Trees& res);

	//for measuring numeric properties

	//how many nodes of type X between me and root?
	static int depth(Tree* node, stIndex label);
	static int wordsDominated(Tree* node);
	//how many words come before this node?
	static int wordsBefore(Tree* node);

	static bool appositive(Tree* tree);

	static bool passive(Tree* tree, bool seenAux=false);

	static Tree* determiner(Tree* tree);

//protected:
	void getNPs();
	void getNPs(Tree* tree, TreeSet& foundHeads, int& index);

	Tree* _tree;
	int _index;
	int _speaker;
	int _dialogue;
	int _time;
	intSet _mentioned;
	Document* _parent;
	NPs _nps;
};

ostream& operator <<(ostream& os, Sent&);
ostream& operator <<(ostream& os, Tree&);
istream& operator >>(istream& is, Sent&);

#endif
