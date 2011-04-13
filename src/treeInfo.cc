#include "treeInfo.h"
#include "ntInfo.h"
#include "setLabs.h"
#include "headFinder.h"
#include "assert.h"
#include "Sent.h"

stIndex
woneof(Tree* post, stIndex* labs,int n)
{
  assert(post);
  Tree* p=post->subtrees;
  assert(p);
  int i;
  for(i=0;i<n;i++) if(p->label==labs[i]) return p->label;
  return 0;
}

int
isHas(Tree* t)
{
  return woneof(t,haves,4);
}

int
isBe(Tree* t)
{
  return woneof(t,bes,7);
}

int
isDo(Tree* t)
{
  return woneof(t,dos,4);
}

int
isVBG(Tree* t)
{
  assert(t);
  if(t->label==vbgLabel)return 1;
  if(t->label==auxgLabel)return 1;
  return 0;
}

int
isVBN(Tree* t)
{
  assert(t);
  if(t->label==vbnLabel)return 1;
  if(woneof(t,vbns,4))return 1;
  return 0;
}

void
excise(Tree* t,Tree* e)
{
  Tree* p=t->subtrees;
  if(p==e){
    t->subtrees=e->sibling;
    return;
  }
  for(;p->sibling;p=p->sibling)
    if(p->sibling==e){
      p->sibling=e->sibling;
      return;
    }
  t->htree=t->hTreeFromTree();
  return;
}

void
insert_before(Tree* t,Tree* aft,Tree*& insrt)
{
  Tree* p=t->subtrees;
  insrt->parent=t;
  insrt->sibling=aft;
  if(p==aft){
    t->subtrees=insrt;
    return;
  }
  for(;p;p=p->sibling)
    if(p->sibling==aft){
      p->sibling=insrt;
      return;
    }
  t->htree=t->hTreeFromTree();
  return;
}  

int
isNoun(stIndex i)
{
  return(i==nnLabel||i==nnsLabel||i==nnpLabel||i==nnpsLabel);
}

int
ccTree(Tree* t)
{
  Tree* p;
  stIndex tlab=t->label;
  int sawcc=0;
  int sawcomma=0;
  int sawcolon=0;
  int pos=0;
  int sawtlab=0;
  for(p=t->subtrees;p;p=p->sibling){
    if(p->label==tlab) sawtlab++;
    else if(pos!=0 && (p->label==ccLabel || p->label==conjpLabel))  sawcc++;
    else if(p->label==commaLabel && pos!=0) sawcomma++;
    else if(p->label==colonLabel && pos!=0) sawcolon++;
    else if(isPunc(p->label)){}
    else return 0;
    pos++;
  }
  if(tlab==npLabel && sawtlab==2 && !sawcc) return 0;
  if((sawcomma || sawcolon || sawcc)&&sawtlab>=2) return 2;
  return 0;
}

/*
void
wtree(Tree* t)
{
  t->write(cerr,gst);
  cerr<<"\n";
}
*/
int
isPron(Tree* t)
{
  if(!t)return 0;
  t=t->subtrees;
  if(!t)return 0;
  stIndex l=t->label;
  int i;
  for(i=0;i<N_PRNS;i++)if(l==prossi[i])return 1;
  return 0;
}

int
thrdper(Tree* t)
{
  if(!t)return 0;
  t=t->subtrees;
  if(!t)return 0;
  stIndex l=t->label;
  int i;
  for(i=0;i<16;i++)if(l==prossi[i])return 1;
  return 0;
}

int
masc(Tree* t)
{
  if(!t)return 0;
  t=t->subtrees;
  if(!t)return 0;
  stIndex l=t->label;
  int i;
  for(i=0;i<3;i++)if(l==prossi[i])return 1;
  return 0;
}

int
fem(Tree* t)
{
  if(!t)return 0;
  t=t->subtrees;
  if(!t)return 0;
  stIndex l=t->label;
  int i;
  for(i=3;i<6;i++)if(l==prossi[i])return 1;
  return 0;
}

int
possessive(Tree* t)
{
  if(!t)return 0;
  if(t->label== prpdLabel)return 1;
  return 0;
}


int
neut(Tree* t)
{
  if(!t)return 0;
  t=t->subtrees;
  if(!t)return 0;
  stIndex l=t->label;
  int i;
  for(i=6;i<8;i++)if(l==prossi[i])return 1;
  return 0;
}
  
int
gen(Tree* t)
{
  int ans=0;
  if(neut(t))ans=2;
  if(fem(t))ans=1;
  return ans;
}
     

int
plr(Tree* t)
{
  if(!t)return 0;
  t=t->subtrees;
  if(!t)return 0;
  stIndex l=t->label;
  int i;
  for(i=11;i<16;i++)if(l==prossi[i])return 1;
  for(i=23;i<27;i++)if(l==prossi[i])return 1;  
  return 0;
}

int
isObj(Tree* t, Tree* obj)
{
  if(!t)return 0;
  Tree* p;
  if(t->label!=vpLabel)return 0;
  for(p=t->subtrees;p;p=p->sibling){
    if(p==obj){
      return 1;
    }
    if(p->label==vpLabel){ //|| p->label==sLabel){
      return isObj(p,obj);
    }
  }
  return 0;
}

int
preposedConstit(Tree* t)
{
  if(!t)return 0;
  stIndex lab=t->label;
  if(lab!=ppLabel && lab!=sbarLabel && lab!=advpLabel
     && lab!=sLabel && !t->vals[relP])return 0;
  Tree* par=t->parent;
  if(!par)return 0;
  if(par->label!=sLabel)return 0;
  Tree* p;
  int foundNP=0;
  for(p=t->subtrees;p;p=p->sibling)
    if(p->label==npLabel)foundNP=1;
  //if(foundNP==0)wtree(par);
  return 1;
}

int
isPronoun(Tree* t)
{
  stIndex l=t->label;
  return(l==prpLabel||l==prpdLabel);
}

int
ge_isPron(Tree* t)
{
  if(!t)return 0;
  t=t->subtrees;
  if(!t)return 0;
  stIndex l=t->label;
  int i;
  for(i=0;i<14;i++)if(l==ge_prossi[i])return 1;
  return 0;
}

int
ge_thrdper(Tree* t)
{
  if(!t)return 0;
  t=t->subtrees;
  if(!t)return 0;
  stIndex l=t->label;
  int i;
  for(i=0;i<8;i++)if(l==ge_prossi[i])return 1;
  return 0;
}

int
ge_masc(Tree* t)
{
  if(!t)return 0;
  t=t->subtrees;
  if(!t)return 0;
  stIndex l=t->label;
  int i;
  for(i=0;i<3;i++)if(l==ge_prossi[i])return 1;
  return 0;
}

int
ge_fem(Tree* t)
{
  if(!t)return 0;
  t=t->subtrees;
  if(!t)return 0;
  stIndex l=t->label;
  int i;
  for(i=3;i<6;i++)if(l==ge_prossi[i])return 1;
  return 0;
}

int
ge_possessive(Tree* t)
{
  if(!t)return 0;
  if(t->label== prpdLabel)return 1;
  return 0;
}


int
ge_neut(Tree* t)
{
  if(!t)return 0;
  t=t->subtrees;
  if(!t)return 0;
  stIndex l=t->label;
  int i;
  for(i=6;i<8;i++)if(l==ge_prossi[i])return 1;
  return 0;
}
  
int
ge_gen(Tree* t)
{
  int ans=0;
  if(ge_neut(t))ans=2;
  if(ge_fem(t))ans=1;
  return ans;
}

int
ge_plr(Tree* t)
{
  if(!t)return 0;
  t=t->subtrees;
  if(!t)return 0;
  stIndex l=t->label;
  int i;
  for(i=8;i<10;i++)if(l==ge_prossi[i])return 1;
  return 0;
}
