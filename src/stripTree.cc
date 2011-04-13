/* Bs 360975 
 6776 found (458 mults), 2713 open 2504 closed 
 7938 1551 1342  Bs: 343884 
 after stripCC 340990 
 5654 1243 1034  340833
 stupd model erasure, left44 mds in firsst 100, removed 266 Bs 338596 
 also removed neg, cut down on bad modals (22 left)  Bs 338016 
 completed strip MD Bs 336248 
 remove POS Bs333575
 removed CCs from flat NPs 332583 
 extended strip MD tohandle ADVPs 332357
 stripPP 310778
 basic infinitival TOs perfect   prgressive  passive  do 304710      
 sbar "that"s 303670,
 the,a 288175  more permissive stripPP 288175,del an 287471
*/ 

#include <fstream>
#include <iostream>
//#include "ECArgs.h"
#include "Tree.h"
#include "headFinder.h"
#include "string.h"
#include "SymbolTable.h"
#include "setLabs.h"
#include "ntInfo.h"
#include "treeInfo.h"

extern SymbolTable* gst;
int OCQ =0;
int OQ=0;
int CQ=0;

int
npccTree(Tree* t)
{
  if(!t)return 0;
  Tree* p=t->subtrees;
  stIndex b=0,a=0,cc=0,l;
  int pos=0;
  for(;p;p=p->sibling){
    l=p->label;
    if(l==ccLabel){
      cc=pos;
    }
    else if(isNoun(l)){
      if(cc)a=l;
      else b=l;
    }
    pos++;
  }
  return(a&&b&&cc?cc:0);
}

void
moveUp(Tree* par, Tree* chi)
{
  par->label=chi->label;
  par->subtrees=chi->subtrees;
  chi->subtrees=NULL;
  chi->sibling=NULL;
  if(!par->reftype)par->reftype=chi->reftype;
  if(!par->refnum)par->refnum=chi->refnum;
  if(!par->numprev)par->numprev=chi->numprev;
  delete(chi);
  Tree* p;
  par->vals[quoteP]=(par->vals[quoteP]||chi->vals[quoteP]);
  for(p=par->subtrees;p;p=p->sibling) p->parent=par;
  par->htree=par->hTreeFromTree();
}

stIndex
removeit(Tree* t, stIndex lab)
{
  if(!t)return 0;
  stIndex ans=0;
  Tree *s=t->subtrees;
  if(!s)return 0;
  if(!s->subtrees)return 0; //don't look at the word ",", only the pos;
  if(s->label==lab && s->sibling){
    t->subtrees=s->sibling;
    s->sibling=NULL;
    ans=s->subtrees->label;
    delete(s);
  }
  for(s=t->subtrees;s;s=s->sibling){
    Tree *st=s->sibling;
    if(!st) continue;
    if(st->label!=lab)continue;;
    s->sibling=st->sibling;
    st->sibling=NULL;
    ans=st->subtrees->label;
    delete(st);
  }
  t->htree=t->hTreeFromTree();
  return ans;
}

stIndex
removeit2(Tree* t, stIndex poslab, stIndex wlab)
{
  /* (t (s wlab) ...) */
  if(!t)return 0;
  stIndex ans=0;
  Tree *s=t->subtrees;
  if(!s)return 0;
  if(!s->subtrees)return 0;
  stIndex sl=s->subtrees->label;
  if(s->label==poslab && sl==wlab && s->sibling){
    t->subtrees=s->sibling;
    s->sibling=NULL;
    ans=s->subtrees->label;
    delete(s);
  }
  /* (t ... Is ...) (st wlab) ...) */
  for(s=t->subtrees;s;s=s->sibling){
    Tree *st=s->sibling;
    if(!st) continue;
    if(!st->subtrees)continue;
    sl=st->subtrees->label;
    if(st->label==poslab&&sl==wlab){
      s->sibling=st->sibling;
      st->sibling=NULL;
      ans=st->subtrees->label;
      delete(st);
    }
  }
  t->htree=t->hTreeFromTree();
  return ans;
}


/* we are looking for (par ... (t ...( ``) ...)))'' */

Tree*
unlevelClosed(Tree* t, Tree* par, Tree*& clo)
{
  if(!par)return NULL;
  Tree *p=par->subtrees;
  for(;p;p=p->sibling){
    if(p==t){
      if(!p->sibling) return unlevelClosed(par,par->parent,clo);
      if(p->sibling->label== closedLabel){clo=p;return par;}
      return NULL;
    }
  }
}

void
stripQuotes(Tree* t)
{
  if(!t)return;
  if(!t->subtrees)return;
  Tree *p=t->subtrees;
  for(;p;p=p->sibling)stripQuotes(p);
  Tree *open=NULL,*closed=NULL;
  p=t->subtrees;
  for(;p->sibling;p=p->sibling){
    if(p->label==openLabel){
      open=p;
      continue;
    }
    Tree* sib=p->sibling;
    if(sib && sib->label==closedLabel){
      closed=p;
      if(!open)break;
      if(open->sibling==p) p->vals[quoteP]=openCloseQ;
      else if(open==t->subtrees&&!sib->sibling) t->vals[quoteP]=openCloseQ;
      else{
	open->sibling->vals[quoteP]=openQ;
	closed->vals[quoteP]=closeQ;
      }
    }
  }
  if(open&&closed){
    OCQ++;
    removeit(t,openLabel);
    removeit(t,closedLabel);
  }
  else if(open&&!closed){
    p=open->sibling;
    if(!p) return;
    Tree* clq;
    Tree* uc=unlevelClosed(t,t->parent,clq);
    if(uc){
      // (t  ... `` sib)))'' 
      if(!p->sibling)p->vals[quoteP]=openCloseQ;
      else{
	// (t  ... `` sib1 sib2 ...)))''
	p->vals[quoteP]=openQ;
	for(;p->sibling;p=p->sibling){}
	p->vals[quoteP]=closeQ;
      }
      removeit(t,openLabel);
      removeit(uc,closedLabel);
      OCQ++;
      CQ--;
    }
    else OQ++;
  }
 else if(!open&&closed)CQ++;
}

void
stackAspect(Tree* t, stIndex i)
{
  t->vals[aspectP]=t->vals[aspectP]*10+infV;
}

void
stripVP(Tree* t)
{
  /*
    t(VP (MD ..) vp(VP ...))
    t(VP (TO ..) vp(VP ...))
  */
  if(!t)return;
  Tree* p;
  for(p=t->subtrees;p;p=p->sibling) stripVP(p);
  if(t->label!=vpLabel)return;
  Tree* md=t->subtrees;
  if(!md)return;
  stIndex l=md->label;
  assert(md->subtrees);
  stIndex mdw=md->subtrees->label;
  Tree *vp=md->sibling;
  if(!vp)return;
  Tree *advp=NULL;
  if(vp->label==advpLabel){
    advp=vp;
    vp=vp->sibling;
  }
  if(!vp)return;
  if(vp->label!=vpLabel) return;
  Tree *vpn=vp->sibling;
  if(vpn) return;
  if(l==mdLabel){}
  else if(l==toLabel){}
  else if(isHas(md)){}
  else if(isBe(md)){}
  else if(isDo(md)){}
  else return;
  if(l==mdLabel)t->vals[modalP]=md->subtrees->label;
  else if(l==toLabel)stackAspect(t,infV);
  else if(isDo(md))stackAspect(t,doV);
  else if(isHas(md))stackAspect(t,perfV);
  else if(isBe(md)&&isVBG(vp->subtrees))stackAspect(t,progV);
  else if(isBe(md)&&isVBN(vp->subtrees)){
    stackAspect(t,passiveV);
  }
  else return;
  removeit(t,l);
  if(advp){
    Tree *par=t->parent;
    assert(par);
    excise(t,advp);
    insert_before(par,t,advp);
  }
  moveUp(t,vp);
}

int
argument(Tree* par, Tree* pp)
{
  assert(pp);
  Tree* in1=pp->subtrees;
  assert(in1);
  if(woneof(in1,argIns,10))return 1;
  return 0;
}

void
stripPP(Tree* t)
{
  if(!t)return;
  Tree*p=t->subtrees;
  if(!p)return;
  for(;p;p=p->sibling)  stripPP(p);
  p=t->subtrees;
  for(;p;p=p->sibling){
    if(p->label!=ppLabel)continue;
    if(!argument(t,p))continue;
    Tree* in1=p->subtrees;
    if(!in1) continue;
    Tree* np=in1->sibling;
    if(!np)continue;
    if(np->sibling)continue;
    if(np->label!= npLabel)continue;
    if(in1->label!=inLabel&&in1->label!=toLabel)continue;
    /* t( ... p(PP in1(IN in) np1(NP ...)) ...) */
    stIndex ppw=removeit(p,in1->label);
    p->vals[relP]=ppw;
    moveUp(p,np);
  }
}
      
void
stripCC(Tree* t)
{
  if(!t)return;
  Tree* p=t->subtrees;
  if(!p) return;
  for(;p;p=p->sibling) stripCC(p);
  if(ccTree(t)){
    removeit(t,ccLabel);
    removeit(t,conjpLabel);
    p=t->subtrees;
    for(;p;p=p->sibling){p->vals[ccP]=1;}
  }
  int pos =npccTree(t);
  if(pos){
    p=t->subtrees;
    int pos2=0;
    for(;p;p=p->sibling){
      stIndex lab=p->label;
      if(isNoun(lab)){
	if(pos2==pos-1||pos2==pos+1)
	p->vals[ccP]=1;
      }
      pos2++;
    }
    removeit(t,ccLabel);
  }
}

void
stripPunc(Tree* t)
{
  if(!t)return;
  Tree* p=t->subtrees;
  if(!p) return;
  for(;p;p=p->sibling) stripPunc(p);
  removeit(t,commaLabel);
  removeit(t,colonLabel);
  stIndex pl=removeit(t,periodLabel);
  if(pl==questionLabel||pl==exclLabel) t->vals[finalPuncP]=pl;
  pl=removeit2(t,rbLabel,notLabel);
  if(!pl)  pl=removeit2(t,rbLabel,ntLabel);
  if(pl)t->vals[negP]=1;
  pl=removeit(t,posLabel);
  if(pl)t->vals[relP]=1;
  if(t->label==sbarLabel){
    removeit2(t,inLabel,thatLabel);
    removeit2(t,dtLabel,thatLabel);
    /* (SBAR subt(WHNP (WP who)) s(S ))) */
    Tree* subt=t->subtrees;
    if(subt && subt->label==whnpLabel){
      Tree* s=subt->sibling;
      if(s && s->label==sLabel && !s->sibling){
      }
    }
  }
  /*
  if(t->label==npLabel){
    if(removeit2(t,dtLabel,theLabel))t->vals[dtP]=theV;
    else if(removeit2(t,dtLabel,aLabel))      t->vals[dtP]=aV;
    else if(removeit2(t,dtLabel,anLabel))      t->vals[dtP]=aV;
  }
  */
}

void
stripTree(Tree* correct)
{
  stripQuotes(correct);
  stripCC(correct);
  stripPunc(correct);
  //stripPP(correct);
  stripVP(correct);
}



