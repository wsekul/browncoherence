#ifndef GE_H
#define GE_H
#include "Tree.h"
#include <map>
#include <vector>

#define ITFAC 50
#define MAXSTORYSZ 2000
#define MAXSENTNPS 200
#define MAXSENTTRS 1000
#define MAXNUMWS 20000
#define NUMTLBS 3
#define NUMSBS 3
#define NUMREFS 15
#define NUMREFVS 6
#define COMEXP 4
typedef pair<int,int> ipair;
typedef map<stIndex,ipair> stIpMap;
typedef map<stIndex,stIpMap> stStMap;
typedef map<stIndex,stIndex> stMap;
typedef stMap::iterator stMapIter;

class Ge
{
 public:
  double procStory(vector<Tree*>& storyTrees);
  void findPrns(Tree* t);
  void trainPrns(Tree* t);
  void addStoryNps(Tree* t);
  void resetStory(int wh);
  void printTrData(SymbolTable* gst);
  void showComb();
  void readProbs(istream& is, SymbolTable* gst);
 private:
  float kld(int w);
  float merit(int w);
  void readSynInfo(istream& ifs,SymbolTable* gst);
  void writeSynInfo(SymbolTable* gst);
  int sentenceB(int n);
  int tildaB(int n);
  void initComb();
  void storeComb(int v,stIndex idx,int hd,int tld,int snt);
  float getComb(stIndex idx,int hd,int tld,int snt);
  float fixPrb(int hdb,int tldb,int sb);
  void resetTrainingStory(int wh);
  int tildaVal(Tree* t);
  void recountTildas(Tree* t);
  void collectG(int g,Tree* t);
  void collectGender(Tree* prn, Tree* np);
  void collectNumber(Tree* prn, Tree* np);
  int tdOrder(Tree* t, Tree** trs);
  int addToRefs(Tree* t, Tree** refs, int rnum);
  int findPastNps(Tree* pastNps[],int numsofar);
  int posRef(Tree* prn, Tree* ref);
  float getNumberProb(int g,int n,Tree* t);
  float getGenderProb(int g,Tree* t);
  void printGenData(SymbolTable* gst);
  stIndex bundleSynInfo(Tree* t,int&iss);
  int sentInternalRefs(Tree* t, Tree* org, Tree* opar, Tree** refs, int rnum, int up);
  void storeSynInfo(int val,int iss, stIndex vb);
  int getSynInfo(int iss, stIndex vb);

  double lgStoryPrb;
  Tree* storyNps[MAXSTORYSZ][MAXSENTNPS];
  int  numStoryNps[MAXSTORYSZ];
  int numSentNps;
  int hdScore[NUMREFVS][2];
  int genders[MAXNUMWS][4];
  float genderps[MAXNUMWS][4];
  int numbers[3][2];
  float numberps[3][2];
  float combps[6][3*COMEXP];
  ipair combs[6][12];
  stMap countMap;
  stMap headMap;
  stStMap synInfo;
};

#endif

