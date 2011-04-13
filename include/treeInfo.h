#ifndef TREEINFO_H
#define TREEINFO_H

#include "Tree.h"

int isHas(Tree* t);
int isBe(Tree* t);
int isDo(Tree* t);
int isVBN(Tree* t);
int isVBG(Tree* t);
Tree* headTree(Tree* t);
int ccTree(Tree* t);
void wtree(Tree* t);
int isPron(Tree* t);
int thrdper(Tree* t);
int masc(Tree* t);
int fem(Tree* t);
int possessive(Tree* t);
int neut(Tree* t);
int gen(Tree* t);
int plr(Tree* t);
int isObj(Tree* t, Tree* obj);
int findSynSubj(Tree* t);
int preposedConstit(Tree* t);
int isPronoun(Tree* t);
int isNoun(stIndex st);
void excise(Tree* t,Tree* e);
void insert_before(Tree* t,Tree* aft,Tree*& insrt);
stIndex woneof(Tree* post, stIndex* labs,int n);

int ge_isPron(Tree* t);
int ge_thrdper(Tree* t);
int ge_masc(Tree* t);
int ge_fem(Tree* t);
int ge_possessive(Tree* t);
int ge_neut(Tree* t);
int ge_gen(Tree* t);
int ge_plr(Tree* t);


#endif
