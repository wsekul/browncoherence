
#include "assert.h"
#include <string.h>
#include <ctype.h>
#include "SymbolTable.h"
#include "ntInfo.h"

short ntInfo[MAXPOSINT];


void
readNtInfo(istream& is, SymbolTable* si)
{
  stIndex sii;
  int info,i;

  for(i = 0 ; i < 500 ; i++) ntInfo[i] = 0;
  for( ; ; ){
    string tmp;
    is >> tmp;
    if(!is)break;
    sii=si->toIndex(tmp);
    is >> info;
    assert(sii < MAXPOSINT);
    ntInfo[sii] = info;
  }
}
