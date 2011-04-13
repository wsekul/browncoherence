
#ifndef STBL_H
#define STBL_H

#include "assert.h"
#include <map>
#include <vector>
#include <string>

using std::string;

typedef unsigned int stIndex;
typedef unsigned int usi;
typedef std::map<string,usi> symMap;
typedef symMap::iterator symMapI;


class SymbolTable
{
 public:
  SymbolTable():fword(0),lword(0){sarray.push_back("");}
  string   toString(stIndex i) {assert(i>0&&i<sarray.size()); return sarray[i];}
  stIndex toIndex(string s){
    symMapI smi=smap.find(s);
    if(smi== smap.end()){
      int nn=sarray.size();
      sarray.push_back(s);
      smap[s]=nn;
      return nn;
    }
    else return smi->second;
  }
  stIndex isThere(string s){
    symMapI smi=smap.find(s);
    if(smi== smap.end())return 0;
    else return smi->second;
  }
  stIndex toindex(string s){
    symMapI smi=smap.find(s);
    if(smi== smap.end()) return 0;
    else return smi->second;
  }
  bool okWord(stIndex sti){return(sti>=fword&&sti<=lword);}
  stIndex fword;
  stIndex lword;
  symMap smap;
  std::vector<string> sarray;
};

#endif
