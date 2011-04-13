#include "Tree.h"
#include <fstream>
#include <sys/resource.h>
#include <iostream>
#include <stdlib.h>
#include "headFinder.h"
#include "ntInfo.h"
#include "treeInfo.h"

int Tree::singleLineReader=1; //set to 0 to read prettyprinted trees.
int Tree::newstory=0;

Tree::Tree(Tree& other):
	numprev(other.numprev),
	refnum(other.refnum),
	reftype(other.reftype),
	gmarker(other.gmarker),
	label(other.label),
	subtrees(NULL),
	sibling(NULL),
	parent(NULL)
{
	for(int j=0;j<10;j++)
	{
		vals[j] = other.vals[j];
	}

	if(other.subtrees)
	{
		subtrees = new Tree(*other.subtrees);

		for(Tree* st = subtrees;
			st != NULL;
			st = st->sibling)
		{
			st->parent = this;
		}
	}

	if(other.sibling)
	{
		sibling = new Tree(*other.sibling);
	}

	htree = hTreeFromTree();
}

Tree*
Tree::
hTreeFromTree()
{
  if(!subtrees)return this;
  if(!subtrees->subtrees)return this;
  else{
    int i,hp=headPosFromTree(this);
    Tree* p=subtrees;
    for(i=0;i<hp;i++)p=p->sibling;
    return p->hTreeFromTree();
  }
  return NULL;
}

void
Tree::
writeVals(ostream& os)
{
  int i;
  bool last = false;
  for(i=0;i<10;i++)
    if(vals[i]){
      if(last) os<<".";
      last=true;
      os<<i<<vals[i];
    }
}


void
Tree::
write(ostream& os, SymbolTable* st)
{
  string lab=st->toString(label);
  if(!subtrees)
  {
	  os << lab;
  }
  else
  {
    Tree* p;
    os<< "(" << lab;
// 	if(refnum > 0)
// 	{
// 		os<<"#"<<refnum;
// 	}

    for(p=subtrees;p;p=p->sibling){
      os<<" ";
      p->write(os,st);
    }
    os<<")";
  }
}

void
skipspaces(istream& is,int& cp,string& buf)
{
  int c;
  for(;;){
    if(cp>=buf.size()){
      if(Tree::singleLineReader)return;
      buf="";
      cp=0;
      getline(is,buf);
      if(!is)return;
    }
    c=buf[cp++];
    if(!isspace(c))break;
  }
  cp--;
}


char* reftypes[5]={"OBJREF","ACTION","SYNTAX","NONLOCOBJ","ENV"};

int
reftypeNum(char *tmp)
{
  int i;
  for(i=0;i<5;i++)
    if(!strcmp(reftypes[i],tmp)) return i+1;
  //fprintf(stderr,"U %s\n",tmp);
  return 0;
}

void
readProStuff(Tree* t, char *s, int brkpt,int n)
{
  int k=0,i;
  char tmp[256];
  for(i=brkpt;i<n;i++){
    if(s[i]=='#'){
      int startnum;
      k=startnum=i+2;
      if(s[i+1]!='-'){
	for(k=i+1;k<n;k++){
	  if(s[k]=='-')break;
	  tmp[k-brkpt-1]=s[k];
	}
	tmp[k-brkpt-1]='\0';
	startnum=++k;
	//fprintf(stderr,"OB %s %i\n",tmp,n);
	t->reftype=reftypeNum(tmp);
      }
      for(;k<n;k++){
	if(s[k]== '~')break;
	tmp[k-startnum]=s[k];
      }
      tmp[k-startnum]='\0';
      t->refnum=atoi(tmp);
    }
    if(s[i]=='~'){
      int starttildanum=i+1;
      for(i=starttildanum;i<n;i++) tmp[i-starttildanum]=s[i];
      tmp[i-starttildanum]='\0';
      int sti=atoi(tmp);
      t->numprev=sti;
      //fprintf(stderr,"ST %i %s \n",sti,s);
    }
  }
}

void
readlabel(istream& is,Tree* t,int& cp,string& buf, SymbolTable* si)
{
  int	 c, brkpt=0,n=0, i=0;
  char s[256];
  for(c= buf[cp++] ;(cp<=buf.size() && !isspace(c) && c!='(' && c!=')');c=buf[cp++]){
    s[n++]=c;
  }
  cp--;
  brkpt=n;
  for (i=1; i<n-1; i++){		// don't look at 1st or last char 
    if (strchr(CATSEP,s[i])) {		// if s[i] is in CATSEP
      brkpt= i;
      break;
    }
  }
  readProStuff(t,s,brkpt,n);
  s[brkpt] = '\0';
  stIndex sti=si->toIndex(s);
  t->label=sti;
}

stIndex
readGWord(istream& is,int& cp, string& buf, SymbolTable* si)
{
  char	 s[MAXLABELLEN];

  int	 c, n=0;
  for(c=buf[cp++];(cp<=buf.size() &&!isspace(c) && (c!='(') && (c!=')'));c=buf[cp++]){
    c=tolower(c);
    s[n++] = c;
    assert(n<MAXLABELLEN-1);		/* leave space for '\0' */
  }  
  s[n] = '\0';
  cp--;
  if(n == 0) return 0;
  stIndex ans = si->toIndex((string)s);
  return ans;
}

void
addParent(Tree* tr,Tree* par)
{
  if(!tr)return;
  if(par) tr->parent=par;
  Tree* p;
  for(p=tr->subtrees;p;p=p->sibling)addParent(p,tr);
}

Tree*
Tree::
make(istream& is, SymbolTable* st)
{
  int cp=0;
  string buf;
  newstory=0;
  if(!singleLineReader)skipspaces(is,cp,buf);
  else getline(is,buf);
  if(!is) return NULL;
  if(buf.empty()&&singleLineReader){
    newstory=1;
    return NULL;
  }
  Tree *ans=readB(is,cp,buf,st);
  addParent(ans,NULL);
  return ans;
}

Tree*
Tree::
readB(istream& is,int& cp, string& buf, SymbolTable* si)
{
  int 	c;
  Tree  *t,*p;
  skipspaces(is,cp,buf);

  c=buf[cp];
  cp++;
  switch (c) {

  case ')': 
    return(NULL); break;	/* empty tree */

  case '(':			/* nonterminal */
    skipspaces(is,cp,buf);
    t = new Tree(0);
    readlabel(is,t,cp,buf,si);
    for(;;){
      t->subtrees = p = readB(is,cp,buf,si);
      if(!terminal_p(t->label) && p &&!p->subtrees){
	delete p;
	t->subtrees=NULL;
      }
      else break;
    }
    while (p) {
      Tree* stree=readB(is,cp,buf, si);
      if(!terminal_p(t->label) && stree &&!stree->subtrees){
	delete stree;
      }
      else{
	p->sibling = stree;
	p = p->sibling;
      }
    }
    assert(t->label);
    t->htree=t->hTreeFromTree();
    return(t); break;
  
  default:		
    cp--;
    skipspaces(is,cp,buf);
    t = new Tree(0);
    t->sibling = t->subtrees=NULL;
    t->label = readGWord(is,cp,buf,si);
    assert(t->label>0);
    return(t);
  }}

