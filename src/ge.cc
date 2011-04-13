/*
   291  (NP .. his (PRN Mr counter 's) ..)
   868  the correct ref, woman, is mistagged NNS
   890  real antecedent is mistagged
   2571 the referent is an NNP but only a modifyer.
*/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "ntInfo.h"
#include "headFinder.h"
#include <map>
#include "ge.h"

#include "Tree.h"
#include "SymbolTable.h"
#include "treeInfo.h"
#include "setLabs.h"
#include "Sent.h"

int sentNo;
int totScore[2];
int trainp;
//extern int pprobs[11];
SymbolTable* gst;
int ge_gen(Tree* t);
int ge_ge_isPron(Tree* t);

#define DATA "/home/ec/nlp/ref/DATA/00/"
int real=2;
#define headTree(t) t->htree
int
ccTreeS(Tree* t)
{
  Tree* p;
  for(p=t->subtrees;p;p=p->sibling){
    if(!p->vals[ccP]) return 0;
  }
  return 1;
}

int
embedder(Tree* par, Tree* t)
{
  Tree* ht=headTree(t);
  Tree* parht=headTree(par);
  if(parht!=ht)return 0;
  if(par->label!=npLabel)return 0;
  if(ccTreeS(par))return 0;
  return 1;
}

int refNums=0;
void
numPrev0(Tree* t)
{
  t->numprev=0;
  Tree* p;
  if(real>1&&!trainp){
    p=t->parent;
    if(p&&embedder(p,t))t->refnum=p->refnum; 
    else t->refnum=refNums++;
  }
  for(p=t->subtrees;p;p=p->sibling)numPrev0(p);
}

double
Ge::
procStory(vector<Tree*>& storyTrees)
{
  int i,sz=storyTrees.size();
  resetStory(1);
  for(i=0;i<sz;i++){
    Tree    *t=storyTrees[i];
    if(real)numPrev0(t);
    findPrns(t);
    addStoryNps(t);
//	cerr<<lgStoryPrb<<"\n";
    sentNo++;
  }
  double ans=lgStoryPrb;
  resetStory(0);
  return ans;
}
  
Tree*
immediateHeadTree(Tree* t)
{  
  if(!t)return NULL;
  Tree* p = t->subtrees;
  if(!p)return NULL;
  if(!p->subtrees)return t;
  Tree* ht=t->htree;
  for(;p;p=p->sibling){
    if(p->htree==ht)return p;
  }
  cerr<<"Bad head pos "<<endl;
  assert(0);
  return NULL;
}

Tree*
baseSemHeadTree(Tree* t)
{
  if(!t)return NULL;
  Tree* p = t->subtrees;
  if(!p)return NULL;
  if(!p->subtrees)return t;
  Tree* ht=t->htree;
  Tree* prv=NULL;
  for(;p;p=p->sibling){
    if(p->label==posLabel) return baseSemHeadTree(prv);
    if(p->htree==ht)return baseSemHeadTree(p);
    prv=p;
  }
  assert(0);
  return NULL;
}

/* collecting training data */
int tildaVals[NUMTLBS]={0,0,0};
int tildaLims[NUMTLBS-1]={1,2};

int sentLims[NUMSBS-1]={2,5};
int refLims[NUMREFVS-1]={1,2,3,5,8};

int
nrefBucket(int n)
{
  int i;
  for(i=0;i<NUMREFVS-1;i++)if(n<refLims[i])return i;
  return i;
}

float
Ge::
kld(int w)
{
  float ans=0;
  int i;
  for(i=0;i<3;i++){
    float ng=genderps[w][i];
    if(!ng)continue;
    ans+=ng*log(ng/genderps[0][i]);
  }
  return ans;
}

float
Ge::
merit(int w)
{
  return kld(w);
}

void
Ge::
readSynInfo(istream& ifs,SymbolTable* gst)
{
  int num,n,i;
  stIndex idx;
  ifs>>num;
  for(n=0;n<num;n++){
    string w,feat;
    ifs>>w;
    ifs>>feat;
    if(feat=="SUBJ")idx=1;
    else if(feat=="OBJ")idx=2;
    else idx=gst->toIndex(feat);
    ipair ip(0,1);
    synInfo[gst->toIndex(w)][idx]=ip;
  }
}

void
Ge::
writeSynInfo(SymbolTable* gst)
{
  stStMap::iterator mi1=synInfo.begin();
  int sz=0;
  for(;mi1!=synInfo.end();mi1++){
    stIpMap& m2=mi1->second;
    stIpMap::iterator mi2=m2.begin();
    for(;mi2!=m2.end();mi2++){
      ipair&ip=mi2->second;
      float tot=(float)(ip.first+ip.second);
      if((float)ip.second/tot>=.5)sz++;
    }
  }
  cerr<<sz<<endl;
  mi1=synInfo.begin();
  for(;mi1!=synInfo.end();mi1++){
    stIpMap& m2=mi1->second;
    stIpMap::iterator mi2=m2.begin();
    for(;mi2!=m2.end();mi2++){
      ipair&ip=mi2->second;
      float tot=(float)(ip.first+ip.second);
      if((float)ip.second/tot>=.5){
	stIndex ind=mi2->first;
	string str;
	if(ind==1)str="SUBJ";
	else if(ind==2)str="OBJ";
	else str=gst->toString(ind);
	cerr<<gst->toString(mi1->first)<<" "<<str<<endl;
      }
    }
  }
}

int
Ge::
sentenceB(int n)
{
  n--;
  int i;
  for(i=0;i<NUMSBS-1;i++)if(n<sentLims[i])return i;
  return i;
}
    
int 
Ge::
tildaB(int n)
{  
  n--;  //minus 1 to make this zero based.
  int i;
  for(i=0;i<NUMTLBS-1;i++)
    if(n<tildaLims[i]) return i;
  return i;
}
void
Ge::
initComb()
{
  int hdb,tldb,sb;
  for(sb=0;sb<3;sb++){
    for(hdb=0;hdb<6;hdb++){
      for(tldb=0;tldb<COMEXP;tldb++){
	ipair& ip = combs[hdb][tldb+COMEXP*sb];
	ip.first=0;
	ip.second=0;
      }
    }
  }
}

void
Ge::
storeComb(int v,stIndex idx,int hd,int tld,int snt)
{
  int hdb=nrefBucket(hd);
  int tldb=tildaB(tld);
  tldb++;
  if(tldb==1&&idx!=nnpLabel)tldb=0;
  int sb=sentenceB(snt);
  ipair& ip= combs[hdb][tldb+COMEXP*sb];
  if(v){ip.second++;}
  else ip.first++;
}

float
Ge::
getComb(stIndex idx,int hd,int tld,int snt)
{
  int hdb=nrefBucket(hd);
  int tldb=tildaB(tld);
  tldb++;
  if(tldb==1&&idx!=nnpLabel)tldb=0;
  int sb=sentenceB(snt);
  float ans=combps[hdb][tldb+COMEXP*sb];
  return ans;
}

float
Ge::
fixPrb(int hdb,int tldb,int sb)
{
  if(tldb>0 && combps[hdb][tldb-1+COMEXP*sb]>0) 
    return 2.0*combps[hdb][tldb-1+COMEXP*sb];
  else if(hdb>0 && combps[hdb-1][tldb+COMEXP*sb]>0)
    return 0.5*combps[hdb-1][tldb+COMEXP*sb];
  else return .000001;
}  

void
Ge::
showComb()
{
  int hdb,tldb,sb;
  int totSuc=0;
  for(sb=0;sb<3;sb++){
    cerr<<"\nSENTB="<<sb<<endl;
    for(hdb=0;hdb<6;hdb++){
      cerr<<hdb;
      for(tldb=0;tldb<COMEXP;tldb++){
	ipair& ip = combs[hdb][tldb+COMEXP*sb];
	int tot=ip.first+ip.second;
	float prb=(float)ip.second/(float)tot;
	if(prb==0)prb=fixPrb(hdb,tldb,sb);
	cerr<<"\t"<<prb;
	totSuc+=ip.second;
	combps[hdb][tldb+COMEXP*sb]=prb;
      }
      cerr<<endl;
    }
  }
}

void
Ge::
resetTrainingStory(int wh)
{
  int i,j;
  for(i=0;i<NUMTLBS;i++)tildaVals[i]=0;
  if(wh){
    for(i=0;i<MAXNUMWS;i++){
      for(j=0;j<4;j++){
	genders[i][j]=0;
	genderps[i][j]=0;
      }
      genderps[i][3]=-999999;
    }
    for(i=0;i<NUMREFVS;i++)hdScore[i][0]=hdScore[i][1]=0;
    for(i=0;i<3;i++)for(j=0;j<2;j++)numbers[i][j]=0;
  }
}

  
int
Ge::
tildaVal(Tree* t)
{
  if(!t)return 0;
  if(t->numprev>0){
    return t->numprev;
  }
  Tree* iht=immediateHeadTree(t);
  if(iht && iht!=t) return tildaVal(iht);
  return 0;
}

void
Ge::
recountTildas(Tree* t)
{
  if(!t)return;
  int tildav =t->numprev;
  if(tildav==0)tildav=1;
  tildaVals[tildaB(tildav)]++;
  if(tildav>1){
    //fprintf(stderr,"TV %i %i %i\n",tildav,tildaB(tildav),tildaB(tildav-1));
    int tbm1=tildaB(tildav-1);
    tildaVals[tbm1]--;
    if(tildaVals[tbm1]<0)
      tildaVals[tbm1]=0;

  }
}

void
Ge::
collectG(int g,Tree* t)
{
  stIndex lab=t->label;
  if(lab==prpdLabel) return; // PRP$'s do not indicate the geneder of what they mod;
  if(terminal_p(lab)){
    stIndex w=t->subtrees->label;
    assert(w<MAXNUMWS);
    genders[w][g]++;
  }
}

void
Ge::
collectGender(Tree* prn, Tree* np)
{
  Tree* hdt=baseSemHeadTree(np);
  /* hdt==,e.g., (NNP George) */
  np=hdt->parent;  //should be base NP.
  Tree* p;
  int g=ge_gen(prn);
  genders[0][g]++;
  for(p=np->subtrees;p;p=p->sibling)collectG(g,p);
}

void
Ge::
collectNumber(Tree* prn, Tree* np)
{
  int g=ge_gen(prn);
  Tree* hdt=baseSemHeadTree(np);
  if(ge_isPron(hdt))return;
  stIndex l=hdt->label;
  int n=1;
  if(l==nnpLabel || l==nnLabel) n=0;
  //fprintf(stderr,"%i %i",g,n);
  //wtree(prn);
  //wtree(np);
  numbers[g][n]++;
}

/* end collecting training data */
int
isSubj(Tree* par,Tree* t)
{
  if(!par)return 0;
  if(t->label!=npLabel)return 0;
  if(par->label!=sLabel)return 0;
  if(t->vals[relP])return 0;
  Tree* p;
  for(p=par->subtrees;p;p=p->sibling){
    if(p==t)return 1;
    if(p->label==vpLabel)return 0;
  }
  return 1;
}

int
subObjPair(Tree* t, Tree* org)
{
  Tree* par=t->parent;
  if(!isSubj(par,t))return 0;
  Tree* p;
  for(p=par->subtrees;p;p=p->sibling)
    if(p->label==vpLabel){
      int iso= isObj(p,org);
      //if(iso)cerr<<"ISO"<<endl;
      return iso;
    }
  return 0;
}
  
int
embeddedNp(Tree* t)
{
  if(!t)return 0;
  if(t->label!=npLabel)return 0;
  Tree* par=t->parent;
  if(!par)return 0;
  if(par->label!=npLabel)return 0;
  if(par->subtrees!=t)return 0;
  if(ccTreeS(par))return 0;
  if(par->htree==par->subtrees->htree) return 0;
  if(headTree(t)==headTree(par)) return 1;
  return 0;
}

void
Ge::
resetStory(int wh)
{
  int i,j;
  for(i=0;i<MAXSTORYSZ;i++)for(j=0;j<MAXSENTNPS;j++)storyNps[i][j]=NULL;;
  if(wh&&trainp)initComb();
  if(trainp)resetTrainingStory(wh);
  sentNo=0;
  refNums=0;
  countMap.clear();
  headMap.clear();
  lgStoryPrb=0;
}

int
Ge::
tdOrder(Tree* t, Tree** trs)
{
  int sentTreeCnt=0;
  trs[sentTreeCnt++]=t;
  int sentTreePos=0;
  for(;;){
    if(sentTreePos==sentTreeCnt)break;
    assert(sentTreePos<MAXSENTTRS);
    Tree* ntree=trs[sentTreePos++];
    Tree* p;
    for(p=ntree->subtrees;p;p=p->sibling)
      trs[sentTreeCnt++]=p;
  }
  return sentTreeCnt;
}

int
validNp(Tree* t)
{
  if(t->label!=npLabel)return 0;
  if(real&&!trainp){
    Tree* p=t->parent;
    if(p&&p->refnum==t->refnum)return 0;
  }
  return 1;
}

void
Ge::
addStoryNps(Tree* t)
{
  numSentNps=0;
  Tree* treeStack[MAXSENTTRS];
  int sz = tdOrder(t,treeStack);
  int i;
  for(i=0;i<sz;i++){
//	  cerr<<"vetting "<<*treeStack[i];
    if(validNp(treeStack[i])){
//		cerr<<" pass\n";
      assert(numSentNps<MAXSENTNPS);
      /* note that two storyNps at this point can have the same ref num
	 if they have been already established as coref */
      storyNps[sentNo][numSentNps++]=treeStack[i];
    }
// 	else
// 	{
// 		cerr<<" fail\n";
// 	}
  }
  numStoryNps[sentNo]=numSentNps;
}

int
posref(Tree* cand, Tree* par,Tree* org)
{
  if(!validNp(cand))return 0;
  if(!possessive(org)&&subObjPair(cand,org->parent)) return 0;
  else return 1;
}

void
correctRefInfo(Tree* prev, Tree* t)
{
  if(prev->numprev<t->numprev)prev->numprev=t->numprev;
}

int
Ge::
addToRefs(Tree* t, Tree** refs, int rnum)
{
  Tree* par=t->parent;
  Tree* parht;
  Tree* prev=t;
  for(;par;par=par->parent){
    if(!embedder(par,prev))break;
    prev=par;
  }

  correctRefInfo(prev,t);
  int i;

  for(i=0;i<rnum;i++) if((prev->refnum&& refs[i]
			  &&prev->refnum==refs[i]->refnum)
			 ||prev==refs[i]){

    /*it is possilbe that an np already put on refs is coreferent
      with t, and thus there is no point in putting t on as well*/
    refs[rnum]=NULL;
    return 0;
  }


  refs[rnum]=prev;
  return 1;
}

int
Ge::
findPastNps(Tree* pastNps[NUMREFS],int numsofar)
{
  if(numsofar==NUMREFS)return numsofar;
  int i,j,tot=numsofar;
  for(i=sentNo-1;i>=0;i--){
    if(tot==NUMREFS)break;
    int numforS=numStoryNps[i];
    for(j=0;j<numforS;j++){
      if(tot==NUMREFS)break;
      Tree* pnp=storyNps[i][j];
      assert(pnp);
      if(addToRefs(pnp,pastNps,tot))tot++;
      if(pnp->label>2000000){
	assert(pnp->label<2000000);
      }
    }
  }
  return tot;
}

int
Ge::
posRef(Tree* prn, Tree* ref)
{
  /*prn == (PRP he),eg. ref == (NP ...) */
  assert(prn->subtrees);
  Tree* hd =baseSemHeadTree(ref);
  if(!hd)return 0;
  stIndex hdl=hd->label;
  if(ge_thrdper(hd)){
    if(ge_masc(hd)&& ge_masc(prn)) return 1;
    else if(ge_fem(hd)&& ge_fem(prn)) return 1;
    else if(ge_neut(hd)&& ge_neut(prn)) return 1;
    else if(ge_plr(hd) && ge_plr(prn)) return 1;
    else return 0;
  }
  if(!ge_plr(prn)){
    if(!ge_neut(prn) && hdl!=nnLabel && hdl!=nnpLabel) return 0;
    //if(ccTreeS(ref)) return 0;
    return 1;
  }
  if(ccTreeS(ref)) return 1;
  if(hdl==nnsLabel || hdl==nnpsLabel) return 1;
  return 0;
}  

int
npTreeRN(Tree* nptree)
{
  if(!nptree)return 0;
  if(nptree->refnum>0) return nptree->refnum;
  Tree* iht=immediateHeadTree(nptree);
  if(iht && iht!=nptree) return npTreeRN(iht);
  return 0;
}

int
npTreeTN(Tree* nptree)
{
  if(!nptree)return 0;
  if(nptree->numprev>0) return nptree->numprev;
  Tree* iht=immediateHeadTree(nptree);
  if(iht && iht!=nptree) return npTreeTN(iht);
  return 0;
}

float
Ge::
getNumberProb(int g,int n,Tree* t)
{
  assert(n==0);
  Tree* hdt=baseSemHeadTree(t);
  if(ge_isPron(hdt))return 1;
  stIndex hdl=hdt->label;
  int tnum=1;
  if(ccTreeS(t)){}
  else if(hdl==nnLabel || hdl==nnpLabel)tnum=0;
  //cerr<<g<<" "<<tnum<<" "<<hdl<<" "<<nnpLabel<<" ";
  //wtree(hdt);
  return numberps[g][tnum];
}

float
Ge::
getGenderProb(int g,Tree* t)
{
  Tree* hdt=baseSemHeadTree(t);
  Tree* np=hdt->parent;
  Tree* p;
  float maxmer=-999999;
  stIndex maxw=0;
  for(p=np->subtrees;p;p=p->sibling){
    if(terminal_p(p->label)){
      stIndex w=p->subtrees->label;
      if(w>=MAXNUMWS)continue;
      if(np->label==npLabel && p->label==prpdLabel)continue;
      float mer=genderps[w][3];
      if(mer>maxmer)
	{
	  maxw=w;
	  maxmer=mer;
	}
    }
  }
  float ans=genderps[maxw][g];
  return ans;;
}

stIndex
Ge::
bundleSynInfo(Tree* t,int&iss)
{
  if(!t->subtrees)return 0;
  Tree* par=t->parent;
  Tree* ppar=par?par->parent:NULL;
  Tree* pppar=ppar?ppar->parent:NULL;
  if(pppar){
    iss=isSubj(ppar,par);
    if(!iss) iss=2*isObj(ppar,par);//subj=1, obj=2, pps > 2;
    Tree* vtree=NULL;
    if(iss)vtree=headTree(ppar);
    if(ppar->label==ppLabel){
      iss=headTree(ppar)->subtrees->label;
      vtree=headTree(pppar);
    }
    if(!vtree)return 0;
    stIndex vlab=vtree->subtrees->label;
    return vlab;
  }
  else return 0;
}

void
Ge::
storeSynInfo(int val,int iss, stIndex vb)
{
  ipair& ip=synInfo[vb][iss];
  if(val)ip.second++;
  else ip.first++;
}

int
Ge::
getSynInfo(int iss, stIndex vb)
{
  ipair& ip=synInfo[vb][iss];
  if(ip.second>0 && ip.second>=ip.first)return 1;
  else return 0;
}

void
Ge::
findPrns(Tree* t)
{
  int i;
  if(!t)return;
  int done=0;

  if(!done && isPronoun(t)&& ge_thrdper(t)){
    Tree* pastNps[2*NUMREFS];
    for(i=0;i<2*NUMREFS;i++)pastNps[i]=NULL;
    int proRefNum=t->refnum;
    int sentB=sentenceB(sentNo);
    int numsofar=sentInternalRefs(t,t,t,pastNps,0,1);
    numsofar=findPastNps(pastNps,numsofar);
    float refProbs[2*NUMREFS];
    for(i=0;i<2*NUMREFS;i++)refProbs[i]=0;
    for(i=0;i<numsofar;i++){
      Tree* ntree=pastNps[i];
      if(!ntree){
	continue;
      }
      if(!posRef(t,ntree)){
	refProbs[i]=0;
      }
      else{
	int ntrn=ntree->refnum;;
	int ntp=npTreeTN(ntree);

	float gnp=getNumberProb(ge_gen(t),ge_plr(t),ntree);
	float gp=getGenderProb(ge_gen(t),ntree);
	Tree* nthd=headTree(ntree);
	stIndex ntlb=0;
	if(nthd)ntlb=nthd->label;
	float cnb=getComb(ntlb,i,ntree->numprev,sentNo);
	float v2=gp*gnp*cnb;
	refProbs[i]=v2;
	if(0){
	  cerr<<i<<"\t"<<refProbs[i]<<" "<<cnb<<" "<<gnp<<" "<<gp<<" "<<ntree->numprev;
//	  ntree->write(cerr,gst);
	  cerr<<endl;
	}
      }
    }
    int foundRefNum=-1;
    float maxP=0;
    float totP=0;
    for(i=0;i<NUMREFS;i++){
      float p=refProbs[i];
      if(p>maxP){
	maxP=p;
	foundRefNum=i;
      }
      totP+=p;
    }
    if(foundRefNum>=0){
      if(real){
	stIndex rn= pastNps[foundRefNum]->refnum;
	t->refnum=rn;
	t->pprob=maxP/totP;
	if(t->pprob>=.45){
	  t->numprev=pastNps[foundRefNum]->numprev++;
	  if(rn>0)countMap[rn]++;
	}
	//pprobs[(int)(10*t->pprob)]++;
      }
      int ntrn=pastNps[foundRefNum]->refnum;;
      /* just to insure we did not accidently include the pronoun in the past NPs*/
      assert(pastNps[foundRefNum]!=t);
      lgStoryPrb+=log(refProbs[foundRefNum]);
// 	  cerr<<"\t"<<lgStoryPrb<<" "<<foundRefNum<<"\n";
// 	  cerr<<"\t"<<*t<<"\n";
// 	  cerr<<*pastNps[foundRefNum]<<"\n";
    }

    else if(t->subtrees->label==itLabel){
      int iss=0;
      stIndex vlab=bundleSynInfo(t,iss);
      int ans=0;
      if(vlab>0){
	/*
	ans=getSynInfo(iss,vlab);
	if(ans){
	  done=1;
	  lgStoryPrb-=10;
	  //lgStoryPrb+=log(ans);
	  //t->pprob=1;
	}
	*/
	lgStoryPrb-=10;
      }
      else lgStoryPrb-=100;
    }

    else lgStoryPrb-=100;
  }
  if(validNp(t)&&!isPronoun(t)&&!ccTree(t)){
    Tree* ht=headTree(t);
    assert(ht);
    stMapIter hmi=headMap.find(ht->subtrees->label);
    if(isPronoun(ht)){}
    else if(hmi!=headMap.end()){
      stIndex rn=hmi->second;
      stIndex v=++countMap[rn];
      if(real){
	t->numprev=v;
	t->refnum=rn;
      }
    }
    else{
      stIndex rn=t->refnum;
      if(t->numprev<2){
	headMap[ht->subtrees->label]=t->refnum;
	countMap[t->refnum]=1;
      }
    }
  }
  Tree* p;
  for(p=t->subtrees;p;p=p->sibling)
    findPrns(p);
}

void
Ge::
trainPrns(Tree* t)
{
  int i;
  if(!t)return;
  Tree* tst=t->subtrees;
  if(tst && tst->label==itLabel){
    int iss=0;
    stIndex vlab=bundleSynInfo(t,iss);
    if(vlab>0)storeSynInfo(t->reftype>1,iss,vlab);
  }
  
  if(isPronoun(t)&& ge_thrdper(t)&&t->reftype==1){
    Tree* pastNps[2*NUMREFS];
    for(i=0;i<2*NUMREFS;i++)pastNps[i]=NULL;
    int proRefNum=t->refnum;
    assert(proRefNum!=0);
    int sentB=sentenceB(sentNo);
    /* collect p(A|#timesmentioned) counts */
    int nprev=t->numprev;
    if(nprev<2){
      fprintf(stderr,"FUNNY pron\n");
    }
    nprev--; // the pronoun will be numbered 2 if there was one previous mention.
    int tilB=tildaB(nprev);
    int numsofar=sentInternalRefs(t,t,t,pastNps,0,1);
    numsofar=findPastNps(pastNps,numsofar);
    int seen[NUMREFS];
    for(i=0;i<NUMREFS;i++)seen[i]=0;
    int alreadySeen=0;
    for(i=0;i<numsofar;i++){
      Tree* ntree=pastNps[i];
      if(!ntree)continue;
      int ntrn=npTreeRN(ntree);
      int scr=(ntrn==proRefNum);
      if(scr){
	if(alreadySeen)continue;
	alreadySeen=1;
      }
      int ibuk=nrefBucket(i);
      hdScore[ibuk][scr]++;
      if(scr){
	int spos;
	for(spos=0;spos<i;spos++)if(ntrn==seen[spos])break;
	if(spos==i){
	  collectGender(t,ntree);
	  collectNumber(t,ntree);
	}
      }
      Tree* nthd=headTree(ntree);
      stIndex ntlb=0;
      if(nthd)ntlb=nthd->label;
      storeComb(scr,ntlb,i,ntree->numprev,sentNo);
      seen[i]=ntrn;
    }
  }
  if(trainp && (t->label==prpLabel || t->label==npLabel|| t->label==prpdLabel))
    recountTildas(t);
  Tree* p;
  for(p=t->subtrees;p;p=p->sibling)
    trainPrns(p);
}

Tree*
prevsib(Tree* par,Tree* chi)
{
  if(par==chi)return NULL;
  Tree* p=par->subtrees;
  if(!p)return NULL;
  if(p==chi)return NULL;
  for(;p;p=p->sibling){
    if(p->sibling==chi)return p;
    if(!p->sibling)return p;
  }
  return NULL;
}

int
Ge::
sentInternalRefs(Tree* t, Tree* org, Tree* opar, Tree** refs, int rnum, int up)
{
  if(!t) return rnum;
  Tree* sibs[128];
  int sibcnt=0;
  Tree* p;
  for(p=prevsib(t,opar);p;p=prevsib(t,p)){
    assert(sibcnt<128);
    sibs[sibcnt++]=p;
    Tree* sib=p;
    if(rnum>=NUMREFS) return rnum;
    if(posref(sib,t,org)){if(addToRefs(sib,refs,rnum))rnum++;}
    if(rnum>=NUMREFS) return rnum;
  }
  int seenop=0;
  if(preposedConstit(opar))
    for(p=t->subtrees;p;p=p->sibling){
      if(p==opar){
	seenop=1;
	continue;
      }
      if(seenop){
	assert(sibcnt<128);
	sibs[sibcnt++]=p;
	Tree* sib=p;
	if(rnum>=NUMREFS) return rnum;
	if(posref(sib,t,org)) if(addToRefs(sib,refs,rnum))rnum++;
	if(rnum>=NUMREFS) return rnum;
      }
    }
  int i;
  for(i=0;i<sibcnt;i++){
    rnum=sentInternalRefs(sibs[i],org,t,refs,rnum,0);
    if(rnum>=NUMREFS) return rnum;
  }
  if(!up) return rnum;
  return sentInternalRefs(t->parent,org,t,refs,rnum,1);
}

void
Ge::
printGenData(SymbolTable* gst)
{
  int i,j,numGen=0;
  for(i=0;i<MAXNUMWS;i++){
    float sum0=(float)(genders[i][0]+genders[i][1]+genders[i][2]);
    if(sum0>0)numGen++;
  }
  cerr<<numGen<<endl;
  float sum0;
  for(i=0;i<MAXNUMWS;i++){
    sum0=(float)(genders[i][0]+genders[i][1]+genders[i][2]);
    if(sum0==0)continue;
    for(j=0;j<3;j++)genderps[i][j]=((genders[i][j])+genderps[0][j])/(sum0+1);
    genderps[i][3]=merit(i);
  }
  for(i=0;i<MAXNUMWS;i++){
    sum0=(float)(genders[i][0]+genders[i][1]+ITFAC*genders[i][2]);
    if(sum0==0)continue;
    for(j=0;j<3;j++)
      genderps[i][j]=(((j==2?ITFAC:1)*genders[i][j])+genderps[0][j])/(sum0+1);
  }
  genderps[0][3]=-99999;
  sum0=(float)(genders[0][0]+genders[0][1]+ITFAC*genders[0][2]);
  for(j=0;j<3;j++)genderps[0][j]=(j==2?ITFAC:1)*genders[0][j]/sum0;
  for(i=0;i<MAXNUMWS;i++){
    float sum0=(float)(genders[i][0]+genders[i][1]+ITFAC*genders[i][2]);
    if(sum0==0)continue;
    if(i==0)cerr<<"DUMMY";
    else cerr<<gst->toString(i);
    for(j=0;j<4;j++)cerr<<"\t"<<genderps[i][j];
    cerr<<endl;
  }
}

void
Ge::
printTrData(SymbolTable* gst)
{
  int i,j;
  cerr<<"NUMBERS\n";
  for(i=0;i<3;i++){
    int tot=0;
    for(j=0;j<2;j++)tot+=numbers[i][j];
    for(j=0;j<2;j++){
      numberps[i][j] = (float)numbers[i][j]/(float)tot;
      cerr<<i<<" "<<j<<" "<<numberps[i][j]<<endl;
    }
    cerr<<endl;
  }
  printGenData(gst);
  writeSynInfo(gst);
}

void
Ge::
readProbs(istream& ifs, SymbolTable* gst)
{
  int i,j,k;
  assert(ifs);
  float prb;
  string str;
  ifs>>str;
  for(i=0;i<3;i++){
    for(j=0;j<2;j++){
      ifs>>prb;
      ifs>>prb;
      ifs>>prb;
      numberps[i][j] = prb;
    }
  }
  int numgen,num=0;
  ifs>>numgen;
  for(i=0;i<MAXNUMWS;i++){
    for(j=0;j<4;j++){
      genders[i][j]=0;
      genderps[i][j]=0;
    }
    genderps[i][3]=-999999;
  }
  for(;num<numgen;num++){
    ifs >> str;
    stIndex sti=0;
    if(num!=0)sti=gst->toIndex(str);
    for(j=0;j<4;j++){
      ifs>>prb;
      genderps[sti][j]=prb;
    }
  }
  genderps[0][3]=-99999;
  readSynInfo(ifs,gst);
  for(i=0;i<3;i++){
    ifs>>str;
    for(j=0;j<6;j++)
      for(k=0;k<5;k++){
	assert(ifs);
	ifs>>prb;
	if(k==0)continue;
	combps[j][k-1+COMEXP*i]=prb;
      }
  }
  ifs>>str;
  assert(!ifs);
}

