#include "TimeModel.h"

TimeModel::TimeModel(string& fname)
{
	ifstream ifs(fname.c_str());
	assert(ifs.is_open());
	readMap(_timeHist, ifs);
}

TimeModel::TimeModel(istream& in)
{
	read(in);
}

TimeModel::~TimeModel()
{
}

//two serialization methods: should be inverses of each other

//serializes this class
void TimeModel::write(ostream& os)
{
	os<<"TIME_MODEL\n";
	writeMap(_timeHist, os);
}

//initializes this class -- called by the istream constructor
void TimeModel::read(istream& is)
{
	checkToken(is, "TIME_MODEL");
	readMap(_timeHist, is);
}

//cache info about a document
void TimeModel::initCaches(Document& doc)
{
}

void TimeModel::clearCaches()
{
}

//log probability of a permuted document
//or does training
Prob TimeModel::permProbability(Document& doc, ints& perm, bool train)
{
	_sentScores.clear();
	_sentScores.resize(perm.size());
	_globalScore = 0;

	if(train)
	{
		cerr<<"Warning: doesn't do training.\n";
		return log(0.0);
	}

	int prev = 0;
	Prob ll = 0;
	int sentCtr = 0;
	foreach(ints, pi, perm)
	{
		Sent* curr = doc[*pi];
		int gap = curr->time() - prev;
		Prob term = log(gapProb(gap));

		//conv. can start unpredictably late in the chat?
		//the condition below was *commented out* for swbd
//		if(sentCtr > 0)
		{
			ll += term;
			_sentScores[sentCtr] = term;
		}

		prev = curr->time();

		++sentCtr;
	}

	return ll;
}

//estimate parameters
void TimeModel::estimate()
{
	cerr<<"Warning: does nothing!\n";
}

Prob TimeModel::gapProb(int gap)
{
	intToProb::iterator bottomOfBucket = _timeHist.lower_bound(gap);
	bottomOfBucket--;
	intToProb::iterator topOfBucket = _timeHist.lower_bound(gap);
	if(topOfBucket == _timeHist.end())
	{
		return 1e-10;
	}

	Prob pr = topOfBucket->second;
	Prob uniPr = 1.0 / (topOfBucket->first - bottomOfBucket->first);
	Prob res = pr * uniPr;

	if(!isProb(res))
	{
		cerr<<"prob of "<<gap<<" bucket "<<bottomOfBucket->first<<" to "<<
			topOfBucket->first<<
			" pr "<<pr<<" uniform in bucket "<<uniPr<<" total "<<res<<"\n";
	}
	assertProb(res);
	return res;
}
