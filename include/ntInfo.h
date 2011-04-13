
#ifndef NTINFO_H
#define NTINFO_H
#include <iostream>

using namespace std;

#define MAXPOSINT 500
#define PHRASAL 0
#define OPENC 1
#define CLOSEDC 2
#define PUNC 3

class SymbolTable;
extern short ntInfo[MAXPOSINT];

#define isPunc(si) (si < 500 && ntInfo[si]>=PUNC)
#define terminal_p(si)  (si < 500 && (ntInfo[si] > 0))
#define closedClass_p(si) (si<500 && ((ntInfo[si]==1)||ntInfo[si]>2))
void readNtInfo(istream& is, SymbolTable* st);

#endif
