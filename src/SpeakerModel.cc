#include "SpeakerModel.h"

SpeakerModel::SpeakerModel(double alpha):
	_alpha(alpha)
{
}

SpeakerModel::SpeakerModel(istream& in)
{
	read(in);
}

SpeakerModel::~SpeakerModel()
{
}

int SpeakerModel::history()
{
	return GLOBAL_HIST;
}

//two serialization methods: should be inverses of each other

//serializes this class
void SpeakerModel::write(ostream& os)
{
	os<<"SPEAKER_MODEL\n";
	os<<_alpha<<"\n";
}

//initializes this class -- called by the istream constructor
void SpeakerModel::read(istream& is)
{
	checkToken(is, "SPEAKER_MODEL");
	is>>_alpha;
//	cerr<<"alpha "<<_alpha<<"\n";
}

//cache info about a document
void SpeakerModel::initCaches(Document& doc)
{
}

void SpeakerModel::clearCaches()
{
}

//log probability of a permuted document
//or does training
Prob SpeakerModel::permProbability(Document& doc, ints& perm, bool train)
{
	_sentScores.clear();
	_sentScores.resize(perm.size());
	_globalScore = 0;

	if(train)
	{
		cerr<<"Warning: doesn't do training.\n";
		return log(0.0);
	}

	bool dirichlet = true;
 	Prob disc = 0;

	intIntMap speakers;
	int total = 0;

	Prob ll = 0;

	foreach(ints, pi, perm)
	{
		Sent* curr = doc[*pi];
		int spk = curr->speaker();
		Prob count = 0;
		if(inMap(speakers, spk))
		{
			count = speakers[spk];
			assert(count > 0);
		}
		else
		{
			if(dirichlet)
			{
				count = _alpha; //dirichlet
			}
			else
			{
				count = _alpha + speakers.size() * disc; //py
			}
		}

		if(dirichlet)
		{
			ll += log(count / (total + _alpha)); //dirichlet
		}
		else
		{
			ll += log(count / (total + _alpha + speakers.size() * disc)); //py
		}
		assert(isfinite(ll));

		speakers[spk] += 1;
		total += 1;
	}

	_globalScore = ll;
	return ll;
}

//estimate parameters
void SpeakerModel::estimate()
{
	cerr<<"Warning: does nothing!\n";
}
