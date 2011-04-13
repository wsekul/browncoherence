
#ifndef TREE_H
#define TREE_H

#include <string.h>
#include <fstream>
#include <sys/resource.h>
#include <iostream>
#include <stdlib.h>
#include "SymbolTable.h"

using std::string;
using std::istream;
using std::ostream;

#define CATSEP		"-=|#~"		/* separates categories */
#define MAXLABELLEN 512
class Tree
{
 public:
  Tree(stIndex i) : numprev(0),refnum(0),reftype(0),gmarker(0),label(i),pprob(0),subtrees(NULL),sibling(NULL),parent(NULL)
    { int j; for(j=0;j<10;j++)vals[j]=0; }

  Tree(istream& is,SymbolTable*st);    
  Tree(Tree& other);
  ~Tree(){
    if(subtrees) delete subtrees;
    if(sibling) delete sibling;
  }
  short       vals[10];
  short       numprev;
  short       refnum;
  short       reftype;
  short       gmarker;
  stIndex     label;
  float       pprob;
  Tree  *subtrees;
  Tree  *sibling;
  Tree  *parent;
  Tree  *htree;
  void  write(ostream& os, SymbolTable* st);
  static Tree* make(istream& is,SymbolTable* st);
  static int singleLineReader; //default=1, set to 0 to read prettyprinted trees.
  static int newstory;
  Tree* hTreeFromTree();
 private:
  static Tree* readB(istream& is,int& cp,string& buf,SymbolTable* st);
  void   writeVals(ostream& os);
};
  

#endif
