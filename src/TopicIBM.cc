#include "TopicIBM.h"
#include "digamma.h"

TopicIBM::TopicIBM(double smooth, int nulls):
	IBM(smooth, nulls)
{
	_alignment.setDimension(1);
}

TopicIBM::TopicIBM(istream& in):
	IBM(0, 0)
{
	read(in);
	VERBOSE = 5;
}
	
TopicIBM::TopicIBM(IBM& other):
	IBM(other)
{
	TopicIBM* ti = dynamic_cast<TopicIBM*>(&other);
	if(ti)
	{
		_topicCounts = ti->_topicCounts;
		_dumpFileNames = ti->_dumpFileNames;
	}
}

void TopicIBM::write(ostream& os)
{
	IBM::write(os);
//	writeHashMap2(_topicCounts, os);
	writeMap(_dumpFileNames, os);
}

void TopicIBM::read(istream& is)
{
	IBM::read(is);

	if(0)
	{
		readHashMap2(_topicCounts, is);
	}

	readMap(_dumpFileNames, is);
}

void TopicIBM::writeCounts(ostream& os)
{
	IBM::writeCounts(os);

	os<<"TOPIC_COUNTS\n";

	writeHashMap2(_newTopicCounts, os);
}
	
void TopicIBM::readCounts(istream& is, intIntMap& vocabTrans)
{
	IBM::readCounts(is, vocabTrans);

	checkToken(is, "TOPIC_COUNTS");

	strStrProbMap dummy;
	readHashMap2(dummy, is);
	foreach(strStrProbMap, fname, dummy)
	{
		_dumpFileNames[fname->first] = _currentDoc;
	}
}

void TopicIBM::getProduced(Sent* sent, NounContexts& res, bool add)
{
	foreach(NPs, np, sent->nps())
	{
		int vind = _vocab.get((*np)->headSym(), add);
		if(vind == -1)
		{
			continue;
		}

		res.push_back(NounContext(vind, (*np)->node()));
		NounContext& created = res.back();
		created.np = *np;
	}
}

Prob TopicIBM::wordGivenPrevWord(NounContext& nextWord, 
								 ContextWord& prevWord, 
								 bool train, Prob totAlign,
								 MaxEntContext& context)
{
	Prob alignProb = _alignment.probToken(prevWord.index, context);

	if(prevWord.np == NULL)
	{
		intProbMap& tCounts = _topicCounts[_currentDoc];
		if(tCounts.empty())
		{
			for(int i = 0; i < _nulls; ++i)
			{
				tCounts[i] = 1;
			}
			tCounts[-1] = _nulls;
		}

		Prob count = tCounts[prevWord.word];
		Prob tot = tCounts[-1];

		Prob pTopic = count/tot;
		assertProb(pTopic);

		//all nulls get the same align prob, and we'll split it up
		//using the more complex multinomial calculated above
		alignProb *= _nulls;
		alignProb *= pTopic;
	}

	if(train && !_estimating)
	{
		getRandBias(prevWord.word, nextWord.word);
	}

	Prob wordProb = transProb(nextWord, prevWord);
	Prob res = wordProb * alignProb;

	return res;
}

void TopicIBM::addPartialCounts(NounContext& nextWord, ContextWord& prevWord,
								MaxEntContext& alignContext, Prob count,
								Prob total,
								ContextWord* max)
{
	if(nextWord.np->pronoun())
	{
		return;
	}

	IBM::addPartialCounts(nextWord, prevWord, alignContext, 
						  count, total, max);
	if(prevWord.np == NULL)
	{
		_newTopicCounts[_currentDoc][prevWord.word] += count;
		_newTopicCounts[_currentDoc][-1] += count;
	}
}

void TopicIBM::normalize()
{
	IBM::normalize();

	_topicCounts = _newTopicCounts;
}

void TopicIBM::smooth()
{
	IBM::smooth();
}

void TopicIBM::initCaches(Document& doc)
{
	_currentDoc = doc.name();

	//XXX
	return;

	if(_topicCounts.find(_currentDoc) == _topicCounts.end() &&
	   !_dumpFileNames.empty())
	{
		string dumpFile = _dumpFileNames[doc.name()];
// 		if(dumpFile == "")
// 		{
// 			cerr<<"Can't find dump for "<<doc.name()<<"\n";
// 			cerr<<doc.size()<<"\n";
// 			cerr<<doc<<"\n";
// 		}
//		assert(dumpFile != "");

		if(dumpFile != "")
		{
			cerr<<"Reading topic counts from "<<dumpFile<<"\n";
			ifstream is(dumpFile.c_str());
			assert(is.is_open());

			{
				string key;

				//_vocab.write(os);
				while(key != ">>")
				{
					is>>key;
				}

				//writeSet(_produced, os);
				while(key != ">>")
				{
					is>>key;
				}

//			writeHashMap(*_newTransTotals, os);
				while(key != ">>")
				{
					is>>key;
				}

				//writeHashMap2(*_newTransCounts, os);
				while(key != ">>>")
				{
					is>>key;
				}
			}

			cerr<<"Done reading dummy items\n";

			checkToken(is, "TOPIC_COUNTS");

			readHashMap2(_topicCounts, is);

			foreach(strIntProbMap, doc, _topicCounts)
			{
				foreach(intProbMap, topic, doc->second)
				{
					if(topic->first == -1)
					{
						continue;
					}

					if(topic->second < 0)
					{
						cerr<<doc->first<<" "<<topic->first<<" "<<
							topic->second<<"\n";
					}
					assert(topic->second >= 0);

					topic->second += _smooth;

					topic->second = exp(digamma(topic->second));
				}

				double priorAdded = (_nulls * _smooth);
				assert(doc->second[-1] >= 0 && priorAdded > 0);
				doc->second[-1] += priorAdded;
				doc->second[-1] = exp(digamma(doc->second[-1]));
			}
			assert(_topicCounts.find(_currentDoc) != _topicCounts.end());
		}
	}
}

void TopicIBM::addNulls(NounContext& prev, string docName)
{
	for(int i = 0; i < _nulls; i++)
	{
		prev.push_back(ContextWord(i, NULL));
	}
}

void TopicIBM::featurizeDistances(NounContext& local, NounContexts& global,
								  bool add)
{
}
