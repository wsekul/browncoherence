#ifndef HEADFIND_H
#define HEADFIND_H
#include "SymbolTable.h"

class SymbolTable;
class Tree;

using namespace std;

void readHeadInfo(istream& hfs,SymbolTable* gst);
int headPosFromTree(Tree* t);
int headPriority(stIndex lhsString, stIndex rhsString, int ansPriority);

#endif				/* ! HEADFIND_H */
