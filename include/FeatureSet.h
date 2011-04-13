class FeatureSet;

#ifndef FEATURE_SET_H
#define FEATURE_SET_H

#include "common.h"

class FeatureSet
{
public:
	FeatureSet();

	void read(istream& is);
	void write(ostream& os);

	void clear();
	unsigned int addType(string ftype, unsigned int flimit);
	unsigned int find(string ftype);

	unsigned int num(unsigned int ftype, unsigned int fval);
    bool isValid(unsigned int fnum);
	void fvalue(unsigned int fnum, unsigned int& ftype, unsigned int& fval);
	void unpair(unsigned int fnum, unsigned int& p1, unsigned int& p2);
	unsigned int pair(unsigned int fnum1, unsigned int fnum2);
	unsigned int triple(unsigned int fnum1, unsigned int fnum2, 
						unsigned int fnum3);
	unsigned int maxFeat();
	string inv(unsigned int fnum);
	unsigned int findBin(unsigned int fnum);
	string realInv(unsigned int fnum);

	strings F_NAMES;
	ints F_LIMITS;
	ints F_BEGIN;
	unsigned int F_LAST;
	unsigned int LAST_BEGIN;
};

#endif
