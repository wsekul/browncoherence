#include "headFinder.h"
#include "assert.h"
#include "ntInfo.h"
#include <fstream>
#include <iostream>
#include "Tree.h"
#include "SymbolTable.h"
#include <map>

extern stIndex ppLabel;

typedef map<stIndex,int> stiMap;
typedef stiMap::iterator stiMapI;
stiMap heads;

void
readHeadInfo(istream& headStrm,SymbolTable* gst)
{
  string next, next2;

  headStrm >> next;
  assert(next=="1");
  int whichHeads = 1;
  for(;;){
    
    headStrm >> next;
    if(!headStrm)break;
    if(next=="2")
      {
	whichHeads = 2;
	continue;
      }
    headStrm >> next2;
    stIndex si1=gst->toIndex(next);
    stIndex si2=gst->toIndex(next2);
    stIndex comb=si1*100+si2;
    heads[comb]=whichHeads;
  }
}

int
headPriority(stIndex lhsIndex, stIndex rhsIndex, int ansPriority) 
{
  stIndex both=lhsIndex*100+rhsIndex;
  int val=0;
  stiMapI stii=heads.find(both);
  if(stii!=heads.end()) val = stii->second;
  
  if(lhsIndex == ppLabel && ansPriority == 1) return 10;//make fst IN head of PP
  if(val==1) return 1;
  else if(ansPriority <= 2) return 10;
  else if(rhsIndex == lhsIndex)
    return 2; //lhs constit. e.g. np -> NP , np;
  else if(val==2) return 3;
  else if(ansPriority == 3) return 10;
  else if(terminal_p(rhsIndex) && !isPunc(rhsIndex)) return 4;
  else if(ansPriority == 4) return 10;
  else if(!terminal_p(rhsIndex) && rhsIndex!=ppLabel)
    return 5;
  else if(ansPriority == 5) return 10;
  else if(!terminal_p(rhsIndex)) return 6;
  else if(ansPriority == 6) return 10;
  else return 7;
}

int
headPosFromTree(Tree* t)
{
  if(!t->subtrees)return 1;
  if(terminal_p(t->label))return 1;
  int   ansPriority = 10;
  stIndex lhsIndex=t->label;
  int   pos = -1;
  int   ans = -1;
  Tree* p;
  for(p=t->subtrees;p;p=p->sibling){
    pos++;
    stIndex rhsIndex=p->label;
    int nextPriority = headPriority(lhsIndex, rhsIndex, ansPriority);
    if(nextPriority <= ansPriority){
      ans = pos;
      ansPriority = nextPriority;
    }
  }
  return ans;
}

