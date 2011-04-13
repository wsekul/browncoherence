#include "FeatureSet.h"

FeatureSet::FeatureSet():
	F_LAST(0),
	LAST_BEGIN(0)
{
}

void FeatureSet::read(istream& is)
{
	checkToken(is, "FSET");
	readVector(F_NAMES, is);
	readVector(F_LIMITS, is);
	readVector(F_BEGIN, is);
	is>>F_LAST;
	LAST_BEGIN = F_BEGIN[F_LAST - 1] + F_LIMITS[F_LAST - 1];
}

void FeatureSet::write(ostream& os)
{
	os<<"FSET\n";
	writeVector(F_NAMES, os);
	writeVector(F_LIMITS, os);
	writeVector(F_BEGIN, os);
	os<<F_LAST<<"\n";
}

void FeatureSet::clear()
{
	F_LAST = 0;
	LAST_BEGIN = 0;
	F_LIMITS.clear();
	F_BEGIN.clear();
	F_NAMES.clear();
}

unsigned int FeatureSet::addType(string ftype, unsigned int flimit)
{
	for(unsigned int ii = 0; ii < F_NAMES.size(); ++ii)
	{
		if(F_NAMES[ii] == ftype)
		{
			cerr<<"Can't add "<<ftype<<" again.\n";
			assert(0);
		}
	}

	unsigned int ntypes = F_NAMES.size();
	F_NAMES.push_back(ftype);
	F_LIMITS.push_back(flimit);

	unsigned int begin = 0;
	if(ntypes > 0)
	{
		unsigned int lastBegin = F_BEGIN[ntypes - 1];
		begin = lastBegin + F_LIMITS[ntypes - 1];
	}
	F_BEGIN.push_back(begin);

	F_LAST = ntypes + 1;
	LAST_BEGIN = begin + flimit;

// 	for(unsigned int i = 0; i < F_BEGIN.size(); ++i)
// 	{
// 		cerr<<F_NAMES[i]<<" "<<F_LIMITS[i]<<" "<<F_BEGIN[i]<<"\n";
// 	}
// 	cerr<<"F_LAST "<<F_LAST<<" "<<LAST_BEGIN<<"\n";

	return ntypes;
}

unsigned int FeatureSet::find(string ftype)
{
	unsigned int ntypes = F_NAMES.size();

	for(unsigned int ii = 0; ii < ntypes; ++ii)
	{
		if(F_NAMES[ii] == ftype)
		{
			return ii;
		}
	}
	cerr<<"Can't find feature type "<<ftype<<"\n";
	assert(0);
}

unsigned int FeatureSet::num(unsigned int ftype, unsigned int fval)
{
	if(ftype > F_LAST)
	{
		cerr<<"No such feature as "<<ftype<<"\n";
	}
	assert(ftype < F_LAST);
	if(fval >= F_LIMITS[ftype])
	{
		cerr<<"Bad value for "<<F_NAMES[ftype]<<": "<<fval<<" (max "<<
			F_LIMITS[ftype]<<")\n";
	}
	assert(fval < F_LIMITS[ftype]);
	return F_BEGIN[ftype] + fval;
}

bool FeatureSet::isValid(unsigned int fnum)
{
	unsigned int tier2 = LAST_BEGIN;
	if(fnum >= tier2)
	{
		unsigned int bpart = fnum / tier2;
		unsigned int secondPart = fnum - bpart * tier2;
		return bpart > secondPart;
	}
	return true;
}

void FeatureSet::fvalue(unsigned int fnum, unsigned int& ftype, 
						unsigned int& fval)
{
	ftype = findBin(fnum);
	fval = fnum - F_BEGIN[ftype];
	if(fval >= F_LIMITS[ftype])
	{
		fval -= F_LIMITS[ftype];
	}
}

void FeatureSet::unpair(unsigned int fnum, unsigned int& p1, unsigned int& p2)
{
	p1 = -1;

	unsigned int tier2 = LAST_BEGIN;
	if(fnum >= tier2)
	{
		p1 = fnum / tier2;
		p2 = fnum - p1 * tier2;
	}
	else
	{
		p2 = fnum;
	}
}

unsigned int FeatureSet::pair(unsigned int fnum1, unsigned int fnum2)
{
	if(fnum1 > fnum2)
	{
		return pair(fnum2, fnum1);
	}
	return fnum1 + LAST_BEGIN * fnum2;
} 

unsigned int FeatureSet::triple(unsigned int fnum1, unsigned int fnum2, 
								unsigned int fnum3)
{
	if(fnum1 > fnum3)
	{
		return triple(fnum3, fnum2, fnum1);
	}
	else if(fnum2 > fnum3)
	{
		return triple(fnum1, fnum3, fnum2);
	}		

	return pair(fnum1, fnum2) +
		LAST_BEGIN * LAST_BEGIN * fnum3;
}

unsigned int FeatureSet::maxFeat()
{
	return LAST_BEGIN * LAST_BEGIN * LAST_BEGIN;
}

string FeatureSet::inv(unsigned int fnum)
{
	unsigned int tier3 = LAST_BEGIN * LAST_BEGIN;

	if(fnum >= tier3)
	{
		unsigned int bpart = fnum / tier3;
		return realInv(bpart) + "-" + inv(fnum - bpart * tier3);
	}
	unsigned int tier2 = LAST_BEGIN;
	if(fnum >= tier2)
	{
		unsigned int bpart = fnum / tier2;
		return realInv(bpart) + "-" + inv(fnum - bpart * tier2);
	}
	return realInv(fnum);
}

unsigned int FeatureSet::findBin(unsigned int fnum)
{
	unsigned int bin;
	for(bin = 0; bin < F_LAST; ++bin)
	{
		if(fnum < F_BEGIN[bin])
		{
			return bin - 1;
		}
	}
	if(fnum < LAST_BEGIN)
	{
		return F_LAST - 1;
	}
	cerr<<"ERROR: feature "<<fnum<<" does not belong to any bin.\n";
	assert(0);
}

string FeatureSet::realInv(unsigned int fnum)
{
	unsigned int bin = findBin(fnum);

	unsigned int val = fnum - F_BEGIN[bin];
	if(val >= F_LIMITS[bin])
	{
		return "noncoref-" + 
			F_NAMES[bin] + intToString(val - F_LIMITS[bin]);
	}
	return F_NAMES[bin] + intToString(val);
}
