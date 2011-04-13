#include "NP.h"
#include "treeInfo.h"

NP::NP(Sent* sent, Tree* node, int index, bool discNewMarkable):
	_parent(sent),
	_node(node),
	_index(index),
	_discNewMarkable(discNewMarkable),
	_markable(false),
	_first(DISC_SINGLE),
	_refnum(-1),
	_prevLink(NULL),
	_appositive(NULL),
	_appositiveTo(NULL)
{
	findRole();
	setRef();

// //	backwards compatible keyword replacement
// 	_normHeadSym = SYMTAB->toIndex(stem(head()));

// 	if(Sent::proper(headSym()))
// 	{
// 		_normHeadSym = SYMTAB->toIndex("_PROPER");
// 	}
// 	else if(contains(MONTHS, headSym()))
// 	{
// 		_normHeadSym = SYMTAB->toIndex("_MONTH");
// 	}
// 	else if(isNum(head()))
// 	{
// 		_normHeadSym = SYMTAB->toIndex("_NUM");
// 	}
// 	else if(isAlphaNum(head()))
// 	{
// 		_normHeadSym = SYMTAB->toIndex("_ALPHA_NUM");
// 	}
// 	else
// 	{
// 		string st = oldStem(head(), posToInt(Sent::headTag(_node)));
// 		_normHeadSym = SYMTAB->toIndex(st);
// 	}

	_normHeadSym = headSym();
}

Sent* NP::parent()
{
	return _parent;
}

int NP::index()
{
	return _index;
}

string NP::fullName()
{
	std::ostringstream ss;
	ss<<_parent->fullName();
	ss<<"-";
	ss<<head();
	ss<<"-";
	ss<<_index;
	string str = ss.str();
	return str;
}

string NP::head()
{
	return Sent::headWord(_node);
}

stIndex NP::headSym()
{
	return Sent::head(_node)->subtrees->label;
}

string NP::normHead()
{
	return SYMTAB->toString(_normHeadSym);
}

stIndex NP::normHeadSym()
{
	return _normHeadSym;
}

int NP::ref()
{
	return _refnum;
}

bool NP::discNewMarkable()
{
	return _discNewMarkable;
}

bool NP::markable()
{
	return _markable;
}

//syntactic role
int NP::role()
{
	return _role;
}

NP* NP::prev()
{
	return _prevLink;
}

Tree* NP::node()
{
	return _node;
}

//discourse-newness status
int NP::first()
{
	return _first;
}

NP* NP::appositive()
{
	return _appositive;
}

NP* NP::appositiveTo()
{
	return _appositiveTo;
}
	
void NP::setStatus(int stat)
{
	_first = stat;
}

void NP::setRef(int ref)
{
	_refnum = ref;
}

void NP::setMarkable(bool mark)
{
	_markable = mark;
}

void NP::setPrev(NP* prev)
{
	_prevLink = prev;
	if(prev)
	{
		setRef(prev->ref());
		setStatus(DISC_OLD);
	}
	else
	{
		setStatus(DISC_SINGLE);
	}
}

bool NP::setAppositive(NP* app)
{
	if(_appositive || app->_appositiveTo)
	{
		_appositive->_appositiveTo = NULL;
		_appositive = NULL;
		app->_appositiveTo = NULL;
		return false;
	}
	_appositive = app;
	app->_appositiveTo = this;
	return true;
}

void NP::print(ostream& os)
{
	os<<Sent::plaintext(node())<<" "<<roleToString(role())<<" "<<
		discToString(first());
	if(ref() != -1)
	{
		os<<" r"<<ref();
	}
//	os<<" "<<Sent::headTag(_node)<<" "<<head();
}

bool NP::pronoun()
{
	return isPron(Sent::head(_node));
}

bool NP::proper()
{
	return Sent::proper(Sent::headTagSym(_node));
}

bool NP::premodifier()
{
	return Sent::preterm(_node);
}

bool NP::definite()
{
	//currently only 'the'
	Tree* lowest = Sent::head(_node)->parent;
	Tree* leftmost = lowest->subtrees;

	if(Sent::word(leftmost->subtrees) == "the" || 
		(leftmost->sibling && 
		 leftmost->sibling->label == dtLabel &&
		 Sent::word(leftmost->sibling->subtrees) == "the")
		)
	{
		return true;
	}
	return false;
}

//'a/an' or no determiner
bool NP::indefinite()
{
	Tree* lowest = Sent::head(_node)->parent;
	Tree* leftmost = lowest->subtrees;

	if(Sent::word(leftmost->subtrees) == "a" ||
	   Sent::word(leftmost->subtrees) == "an")
	{
		return true;
	}
	else if(leftmost->label == pdtLabel || leftmost->label == dtLabel)
	{
		//all the... or the ...
		return false;
	}

	return true;
}

//detects 'the fact that X'
bool NP::hasPropositionArg()
{
	if(!definite())
	{
		return false;
	}

	Tree* sbar = Sent::hasChild(_node, sbarLabel);
	if(sbar)
	{
		if(sbar->subtrees->label == inLabel && 
		   Sent::headWord(sbar->subtrees) == "that")
		{
			//complementizer-that
			return true;
		}

// 		if(Sent::head(sbar)->label != wdtLabel && 
// 		   Sent::head(sbar)->label != wpLabel)
// 		{
// 			return true;
// 		}
	}

	//'the concept of X'
	//overdetects; so far no heuristic to distinguish
	//'the concept of men eating lizards' from
	//'the row of men eating lizards'
// 	Tree* pp = Sent::hasChild(_node, ppLabel);
// 	if(pp)
// 	{
// 		Tree* arg = Sent::hasChild(pp, npLabel);
// 		if(arg)
// 		{
// 			Tree* vp = Sent::hasChild(arg, vpLabel);
// 			if(vp)
// 			{
// 				return true;
// 			}
// 		}
// 	}

	return false;
}

int NP::namedEntityType()
{
	Tree* hd = Sent::head(_node);
	Tree* anc = Sent::ancestor(hd, persLabel);
	if(Sent::isAncestor(anc, _node))
	{
		return NE_PERS;
	}

	anc = Sent::ancestor(hd, locLabel);
	if(Sent::isAncestor(anc, _node))
	{
		return NE_LOC;
	}

	anc = Sent::ancestor(hd, orgLabel);
	if(Sent::isAncestor(anc, _node))
	{
		return NE_ORG;
	}

	anc = Sent::ancestor(hd, pctLabel);
	if(Sent::isAncestor(anc, _node))
	{
		return NE_PCT;
	}

	anc = Sent::ancestor(hd, timeLabel);
	if(Sent::isAncestor(anc, _node))
	{
		return NE_TIME;
	}

	anc = Sent::ancestor(hd, dateLabel);
	if(Sent::isAncestor(anc, _node))
	{
		return NE_DATE;
	}

	anc = Sent::ancestor(hd, mnyLabel);
	if(Sent::isAncestor(anc, _node))
	{
		return NE_MNY;
	}

	return NE_NONE;
}

string NP::roleToString(int role)
{
	switch(role)
	{
		case T_START:
			return "<s>";
		case T_SUBJ:
			return "S";
		case T_OBJ:
			return "O";
		case T_X:
			return "X";
// 		case T_NPMOD:
// 			return "N";
		case T_VERB:
			return "V";
		case T_NONE:
			return "-";
		default:
			std::ostringstream convert;
			convert<<"BUG("<<role<<")";
			return string(convert.str());
//			abort();
	}
}

string NP::discToString(int role)
{
	switch(role)
	{
		case DISC_OLD:
			return "DISC_OLD";
		case DISC_INIT:
			return "DISC_INIT";
		case DISC_SINGLE:
			return "DISC_SINGLE";
		default:
			std::ostringstream convert;
			convert<<"BUG("<<role<<")";
			return string(convert.str());
//			abort();
	}
}

string NP::neToString(int ne)
{
	switch(ne)
	{
		case NE_NONE:
			return "NONE";
		case NE_PERS:
			return "PERSON";
		case NE_LOC:
			return "LOCATION";
		case NE_ORG:
			return "ORGANIZATION";
		case NE_TIME:
			return "TIME";
		case NE_DATE:
			return "DATE";
		case NE_PCT:
			return "PERCENTAGE";
		case NE_MNY:
			return "MONEY";
		default:
			std::ostringstream convert;
			convert<<"BUG("<<ne<<")";
			return string(convert.str());
//			abort();
	}
}

//this function is responsible for syntactic role labeling (S-O-X)
//it is a complete rewrite of the experimental code
// (which was, frankly, insane)
//these heuristics are way simpler, but they sometimes do different stuff
void NP::findRole()
{
	if(_node->parent == NULL)
	{
		_role = T_NONE;
		return;
	}

	Tree* lastNPPar = Sent::lastParent(_node, npLabel);
	Tree* par = lastNPPar->parent;

//  	cerr<<"node: "<<*_node<<"\n";
//  	cerr<<"par: "<<*par<<"\n";
//  	cerr<<"lnpp: "<<*lastNPPar<<"\n";

	if(Sent::sentType(par))
	{
		//mark SUBJ as rightmost NP under S
		_role = T_X;

		Tree* parHC = Sent::headChild(par);
		for(Tree* t = par->subtrees;
			t != NULL;
			t = t->sibling)
		{
			if(t == parHC)
			{
				if(_role == T_SUBJ && Sent::passive(t))
				{
					//passive subjs demoted to obj
					//_role = T_OBJ;
				}
				break;
			}
			else if(Sent::head(t) == Sent::head(_node)
					|| (_node->parent == t && ccTree(t))) //conjoint np
			{
				_role = T_SUBJ;
			}
			else if(t->label == npLabel)
			{
				_role = T_X;
			}
		}

		//if we have sinv, won't find NP subj before head of S
		if(par->label == sinvLabel)
		{
			_role = T_SUBJ;
		}
	}
	else if(par->label == vpLabel)
	{
		//it's an OBJECT if it's an NP directly right of the head
		//under VP under S
		_role = T_OBJ;

		Tree* vp = Sent::lastParent(par, vpLabel);
		Tree* vpPar = vp->parent;
		if(Sent::sentType(vpPar))
		{
			if(Sent::head(vp) != Sent::head(par))
			{
				//XXX replicates the conjoint-vp obj bug
				//cerr<<"Conjoint vp "<<*vp<<"\n";
				//_role = T_X;
			}

			Tree* parHC = Sent::headChild(par);
			parHC = parHC->sibling;
			if(!parHC || 
			   (Sent::head(parHC) != Sent::head(_node)
				&& !(ccTree(parHC) && _node->parent == parHC)))
			{
				//not an OBJ if not adjacent to the head of VP
				_role = T_X;
			}
		}
		else
		{
			_role = T_X;
		}
	}
	else
	{
		//otherwise it's X
		_role = T_X;
	}

// 	if(Sent::appositive(_node))
// 	{
// 		_role = T_NPMOD;
// 	}

//	cerr<<"Setting role "<<roleToString(_role)<<"\n";
}

void NP::setRef()
{
	//see if we can find referent info attached to any of our subheads

	Tree* headT = _node;
	_refnum = headT->refnum;
	while(!Sent::preterm(headT) && _refnum <= 0)
	{
		headT = Sent::headChild(headT);
		_refnum = headT->refnum;
	}

	if(_refnum == 0)
	{
		_refnum = -1;
	}
	else
	{
		_markable = true;
	}
}


ostream& operator <<(ostream& os, NP& np)
{
	np.print(os);
	return os;
}
