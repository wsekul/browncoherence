#include "MentionModel.h"

MentionModel::MentionModel()
{
}

MentionModel::MentionModel(istream& in)
{
	read(in);
}

MentionModel::~MentionModel()
{
}

int MentionModel::history()
{
	return GLOBAL_HIST;
}

//two serialization methods: should be inverses of each other

//serializes this class
void MentionModel::write(ostream& os)
{
	os<<"MENTION_MODEL\n";
	writeHashMap2(_counts, os);
	writeHashMap(_totals, os);
}

//initializes this class -- called by the istream constructor
void MentionModel::read(istream& is)
{
	checkToken(is, "MENTION_MODEL");
	readHashMap2(_counts, is);
	readHashMap(_totals, is);
}

//cache info about a document
void MentionModel::initCaches(Document& doc)
{
}

void MentionModel::clearCaches()
{
}

//log probability of a permuted document
//or does training
Prob MentionModel::permProbability(Document& doc, ints& perm, bool train)
{
	_sentScores.clear();
	_sentScores.resize(perm.size());
	_globalScore = 0;

	intSet speakers;

	Prob ll = 0;

	int prevSpeaker = -1;
	intSet prevMentioned;
	foreach(ints, pi, perm)
	{
		Sent* curr = doc[*pi];
		int spk = curr->speaker();

		int sentType = (spk == prevSpeaker) ? SPK_CONTINUING : SPK_BEGINNING;

		int mentionStatus = NO_MENTION;
		if(!curr->mentioned().empty())
		{
			//mentioned someone-- but who?
			bool known = true;
			foreach(intSet, ment, curr->mentioned())
			{
				if(!contains(speakers, *ment))
				{
					known = false;
					break;
				}
			}

			if(known)
			{
				mentionStatus = START_MENTION;

				if(spk == prevSpeaker)
				{
					bool sameMent = false;
					foreach(intSet, ment, curr->mentioned())
					{
						if(contains(prevMentioned, *ment))
						{
							sameMent = true;
							break;
						}
					}

					if(sameMent)
					{
						mentionStatus = CONTINUE_MENTION;
					}
				}
			}
			else
			{
				mentionStatus = UNKNOWN_MENTION;
			}
		}

		if(train)
		{
			_counts[sentType][mentionStatus] += 1;
			_totals[sentType] += 1;
		}
		else
		{
			Prob term = (_counts[sentType][mentionStatus] + 1e-10) / 
				_totals[sentType];
			assertProb(term);
			ll += log(term);
		}

		prevSpeaker = spk;
		speakers.insert(spk);
		prevMentioned = curr->mentioned();
	}

	_globalScore = ll;
	return ll;
}

//estimate parameters
void MentionModel::estimate()
{
}
