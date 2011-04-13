#include "Sent.h"
#include "treeInfo.h"
#include "ntInfo.h"

Sent::Sent()
{
	_parent = NULL;
	_index = -1;
	_speaker = -1;
	_dialogue = -1;
	_time = -1;
}

Sent::Sent(Sent& other):
	_tree(new Tree(*other._tree)),
	_index(other._index),
	_speaker(other._speaker),
	_dialogue(other._dialogue),
	_time(other._time),
	_mentioned(other._mentioned),
	_parent(NULL)
{
	getNPs();
}

Sent::Sent(Document& doc, int ind, int speaker, int dial, int time, 
		   intSet& mentioned)
{
	_parent = &doc;
	_index = ind;
	_speaker = speaker;
	_dialogue = dial;
	_time = time;
	_mentioned = mentioned;
}

Sent::Sent(Document& doc, int ind, int speaker, int dial, int time, 
		   intSet& mentioned, Tree* tree)
{
	_parent = &doc;
	_index = ind;
	_tree = tree;
	_speaker = speaker;
	_dialogue = dial;
	_time = time;
	_mentioned = mentioned;

	getNPs();
}

Sent::~Sent()
{
	for(NPs::iterator i = _nps.begin();
		i != _nps.end();
		i++)
	{
		delete *i;
	}

	delete _tree;
}

void Sent::read(istream& is)
{
	_tree = Tree::make(is, SYMTAB);
	if(_tree == NULL || _tree->subtrees == NULL)
	{
		_tree = NULL;
	}
	else
	{
		_nps.clear();
		getNPs();
	}
}

void Sent::print(ostream& os)
{
	os<<plaintext(_tree)<<" (";
	if(_speaker > -1)
	{
//		os<<(char)(_speaker+'A')<<" ";
		if(_speaker == 0)
		{
			os<<"A ";
		}
		else
		{
			int sp = _speaker;
			string code;
			while(sp > 0)
			{
				char cpt = 'A' + sp % 26;
				code = cpt + code;
				sp /= 26;
			}
			os<<code<<" ";
		}
	}
	if(_dialogue > -1)
	{
		os<<"D"<<_dialogue<<": "<<_time<<" ";
	}
	os<<fullName()<<")";

// 	os<<"\n";
// 	for(NPs::iterator np = _nps.begin();
// 		np != _nps.end();
// 		np++)
// 	{
// 		os<<**np<<"\n";
// 	}

// 	os<<"\nMentioned: ";
// 	foreach(intSet, ment, _mentioned)
// 	{
// 		int sp = *ment;
// 		string code;
// 		while(sp > 0)
// 		{
// 			char cpt = 'A' + sp % 26;
// 			code = cpt + code;
// 			sp /= 26;
// 		}
// 		os<<code<<" ";
// 	}
// 	os<<"\n";
}

int Sent::index()
{
	return _index;
}

int Sent::speaker()
{
	return _speaker;
}

int Sent::dialogue()
{
	return _dialogue;
}

void Sent::setDialogue(int dial)
{
	_dialogue = dial;
}

int Sent::time()
{
	return _time;
}

intSet& Sent::mentioned()
{
	return _mentioned;
}

Document* Sent::parent()
{
	return _parent;
}

Tree* Sent::tree()
{
	return _tree;
}

string Sent::fullName()
{
	std::ostringstream ss;
	if(parent())
	{
		ss<<parent()->name();
	}
	else
	{
		ss<<"?";
	}
	ss<<"-";
	ss<<index();
	string name = ss.str();
	return name;
}

NPs& Sent::nps()
{
	return _nps;
}

void Sent::getNPs()
{
	if(_tree)
	{
		TreeSet empty;
		int index = 0;
		getNPs(_tree, empty, index);
	}
}

void Sent::getNPs(Tree* tree, TreeSet& foundHeads, int& index)
{
	if(preterm(tree))
	{
		if(tree->label == prpdLabel)
		{
			_nps.push_back(new NP(this, tree, index++, false));
			return;
		}

		//-- mention --
		//marks NPs at all N nodes
		//this is the allNPs option I actually use
		//if you want to test NP-only mention detection, comment out
		//this block

		if(isNoun(tree->label) && !contains(foundHeads, tree))
		{
			_nps.push_back(new NP(this, tree, index++, false));
			return;
		}

		return;
	}	

	if(tree->label == npLabel) //tree is some kind of an np
	{
		if(headTag(tree) == "UH")
		{
			//IRC: if we postcorrect parses by tagging certain words as 'UH'
			//don't worry about NPs they head
		}
 		if(!ccTree(tree)) //X, Y and Z: not markable
		{
			Tree* headT = head(tree);

			//(NP (-> NP foo <-)) isn't markable
			if(!contains(foundHeads, headT))
			{
				foundHeads.insert(headT);

				if(!appositive(tree))
				{
					//found a markable NP
					//cerr<<"Markable "<<*tree<<"\n";
					_nps.push_back(new NP(this, tree, index++, true));
				}
				else
				{
					//record an appositive part as non-markable NP
					//cerr<<"Not markable "<<*tree<<"\n";
					_nps.push_back(new NP(this, tree, index++, false));
				}
			}
		}
	}

	for(Tree* sub = tree->subtrees;
		sub != NULL;
		sub = sub->sibling)
	{
		getNPs(sub, foundHeads, index);
	}
}

bool Sent::appositive(Tree* tree)
{
	//the modifier part of an appositive 
	//(NP (Dan Blake) (,) (-> American Hero <-))
	//isn't markable
	Tree* parent = tree->parent;
	if(parent != NULL)
	{
		//ccTree is pretty good at compound phrases but sometimes
		//misses an obvious case because of a parse error, so
		//currently also checks for a cc in the obvious way
		if(parent->label == npLabel && !ccTree(parent) && 
		   !hasChild(parent, ccLabel) &&
		   hasChild(parent, commaLabel))
		{
			if(headChild(parent) != tree)
			{
				//cerr<<"Appositive part: "<<*parent<<"\t"<<*tree<<"\n";
				return true;
			}
		}
	}
	return false;
}

bool Sent::passive(Tree* tree, bool seenAux)
{
	Tree* aux = hasChild(tree, auxLabel);
	if(aux && isBe(aux))
	{
		seenAux = true;
	}
	Tree* auxg = hasChild(tree, auxLabel);
	if(auxg && isBe(auxg))
	{
		seenAux = true;
	}

	if(hasChild(tree, vbdLabel) || hasChild(tree, vbnLabel))
	{
		return seenAux;
	}

	Tree* vpChild = hasChild(tree, vpLabel);
	if(vpChild)
	{
		return passive(vpChild, seenAux);
	}
	return false;
}

Tree* Sent::determiner(Tree* np)
{
	assert(np->label == npLabel);

	Tree* lowest = Sent::head(np)->parent;
	Tree* leftmost = lowest->subtrees;

	if(leftmost->label == pdtLabel && leftmost->sibling)
	{
		leftmost = leftmost->sibling;
	}

	if(leftmost->label == dtLabel || leftmost->label == prpdLabel)
	{
		return leftmost;
	}
	return NULL;
}

//old implementation; the new tree package doesn't use hpos
// Tree* Sent::headChild(Tree* tree)
// {
// 	if(preterm(tree))
// 	{
// 		return tree->subtrees;
// 	}

// 	int ctr = 0;
// 	for(Tree* i = tree->subtrees;
// 		i != NULL;
// 		i = i->sibling, ctr++)
// 	{
// 		if(ctr == tree->hpos)
// 		{
// 			return i;
// 		}
// 	}

// 	//badness: tree's head pos'n is beyond its last child!
// 	cerr<<"Tree's head pos ("<<tree->hpos<<") is beyond its last child.\n";
// 	cerr<<*tree<<"\n";
// 	abort();
// }

//preterm over the head word
//old implementation; the new tree package provides this
// Tree* Sent::head(Tree* tree)
// {
// 	Tree* headT = tree;
// 	while(!preterm(headT))
// 	{
// 		headT = headChild(headT);
// 	}

// 	return headT;
// }

Tree* Sent::headChild(Tree* tree)
{
	if(preterm(tree))
	{
		return tree->subtrees;
	}

	for(Tree* i = tree->subtrees;
		i != NULL;
		i = i->sibling)
	{
		if(tree->htree == i->htree)
		{
			return i;
		}
	}

	//badness: tree's head pos'n is beyond its last child!
	cerr<<"Tree's head ("<<tree->htree<<") is beyond its last child.\n";
	cerr<<*tree<<"\n";
	abort();
}

string Sent::headWord(Tree* tree)
{
	return word(headChild(head(tree)));
}

string Sent::headTag(Tree* tree)
{
	return tag(head(tree));
}

void Sent::plaintext(Tree* tree, std::ostringstream& os)
{
	if(leaf(tree))
	{
		os<<word(tree);
		os<<" ";
	}
	else
	{
		for(Tree* i = tree->subtrees;
			i != NULL;
			i = i->sibling)
		{
			plaintext(i, os);
		}
	}
}

//returns the words of this tree, as a long string
string Sent::plaintext(Tree* tree)
{
	std::ostringstream os;
	plaintext(tree, os);
	return os.str();
}

//returns the first child of this tree with the specified label
//or null
Tree* Sent::hasChild(Tree* node, stIndex label)
{
	for(Tree* i = node->subtrees;
		i != NULL;
		i = i->sibling)
	{
		if(i->label == label)
		{
			return i;
		}
	}

	return NULL;
}

bool Sent::isAncestor(Tree* dec, Tree* anc)
{
	for(Tree* par = dec;
		par != NULL;
		par = par->parent)
	{
		if(par == anc)
		{
			return true;
		}
	}

	return false;
}	

//first ancestor with label
Tree* Sent::ancestor(Tree* node, stIndex label)
{
	for(Tree* par = node->parent;
		par != NULL;
		par = par->parent)
	{
		if(par->label == label)
		{
			return par;
		}
	}

	return NULL;
}

//last ancestor with label
Tree* Sent::highestAncestor(Tree* node, stIndex label)
{
	Tree* anc = ancestor(node, label);
	Tree* ret = anc;

	while(anc != NULL)
	{
		ret = anc;
		anc = ancestor(anc, label);
	}

	return ret;
}

//returns the first parent of this tree without the specified label
//or null
Tree* Sent::firstParent(Tree* node, stIndex notLabel)
{
	//replace?
// 	Tree* lpar = lastParent(node, notLabel);
// 	if(lpar)
// 	{
// 		return lpar->parent;
// 	}
// 	return NULL;

	for(Tree* par = node->parent;
		par != NULL;
		par = par->parent)
	{
		if(par->label != notLabel)
		{
			return par;
		}
	}

	return NULL;
}

//returns the last parent of this tree with the specified label
//or the node itself
Tree* Sent::lastParent(Tree* node, stIndex label)
{
	Tree* lastPar = node;
	for(Tree* par = node->parent;
		par != NULL;
		par = par->parent)
	{
		if(par->label != label)
		{
			return lastPar;
		}
		else
		{
			lastPar = par;
		}
	}

	return NULL;
}

//places into res all the descendants of node with the specified label
void Sent::collectTrees(Tree* node, stIndex label, Trees& res)
{
	if(node->label == label)
	{
		res.push_back(node);
	}

	for(Tree* i = node->subtrees;
		i != NULL;
		i = i->sibling)
	{
		collectTrees(i, label, res);
	}
}

void Sent::getLeaves(Tree* node, Trees& res)
{
	if(preterm(node))
	{
		res.push_back(node);
	}
	else
	{
		for(Tree* i = node->subtrees;
			i != NULL;
			i = i->sibling)
		{
			getLeaves(i, res);
		}
	}
}

void Sent::getModifiers(Tree* tree, Trees& left, Trees& right)
{
	if(preterm(tree))
	{
		return;
	}

	Tree* headChild = Sent::headChild(tree);
	bool seenHead = false;
	for(Tree* sub = tree->subtrees;
		sub != NULL;
		sub = sub->sibling)
	{
		if(sub == headChild)
		{
			seenHead = true;
			getModifiers(sub, left, right);
		}
		else if(!seenHead)
		{
			left.push_back(sub);
		}
		else
		{
			right.push_back(sub);
		}
	}
}

bool Sent::identical(Tree* one, Tree* two)
{
	if(one->label != two->label)
	{
		return false;
	}

	if(preterm(one))
	{
		if(preterm(two) && two->subtrees->label == one->subtrees->label)
		{
			return true;
		}
		return false;
	}

	Tree* t1 = one->subtrees;
	Tree* t2 = two->subtrees;
	for(;
		t1 != NULL && t2 != NULL;
		t1 = t1->sibling, t2 = t2->sibling)
	{
		if(!identical(t1, t2))
		{
			return false;
		}
	}

	if(t1 != NULL || t2 != NULL)
	{
		return false;
	}
	return true;
}

int Sent::depth(Tree* node, stIndex label)
{
	if(node->parent == NULL)
	{
		return 0;
	}

	if(node->label == label)
	{
		return 1 + depth(node->parent, label);
	}
	else
	{
		return depth(node->parent, label);
	}
}

int Sent::wordsDominated(Tree* node)
{
	if(Sent::leaf(node))
	{
		return 1;
	}
	else
	{
		int res = 0;
		for(Tree* i = node->subtrees;
			i != NULL;
			i = i->sibling)
		{
			res += wordsDominated(i);
		}
		return res;
	}
}

int Sent::wordsBefore(Tree* node)
{
	Tree* par = node->parent;
	int res = 0;

	while(par != NULL)
	{
		for(Tree* i = par->subtrees;
			i != node;
			i = i->sibling)
		{
			res += wordsDominated(i);
		}
		node = par;
		par = par->parent;
	}
	return res;
}

ostream& operator <<(ostream& os, Sent& sent)
{
	sent.print(os);
	return os;
}

ostream& operator <<(ostream& os, Tree& tree)
{
	tree.write(os, SYMTAB);
	return os;
}

istream& operator >>(istream& is, Sent& sent)
{
	sent.read(is);
	return is;
}
