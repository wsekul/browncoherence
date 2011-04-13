class TopicIBM;

#ifndef TOPIC_IBM_H
#define TOPIC_IBM_H

#include "IBM.h"

class TopicIBM : public IBM
{
public:
	TopicIBM(double smooth, int nulls);
	TopicIBM(istream& in);
	TopicIBM(IBM& other);

	//serializes this class
	virtual void write(ostream& os);
	//initializes this class -- called by the istream constructor
	virtual void read(istream& is);

	//writes the expected counts
	virtual void writeCounts(ostream& os);
	//reads in expected counts
	virtual void readCounts(istream& is, intIntMap& vocabTrans);

	virtual void initCaches(Document& doc);

	//gets words that need to be aligned in the next sentence
	virtual void getProduced(Sent* sent, 
							 NounContexts& produced, bool train);

	virtual Prob wordGivenPrevWord(NounContext& nextWord, 
								   ContextWord& prevWord, 
								   bool train, Prob totAlign,
								   MaxEntContext& context);

	virtual void addPartialCounts(NounContext& nextWord, ContextWord& prevWord,
								  MaxEntContext& alignContext, Prob count,
								  Prob total,
								  ContextWord* max);

	virtual void normalize();
	virtual void smooth();

	virtual void featurizeDistances(NounContext& local, 
									NounContexts& global, bool add);
	virtual void addNulls(NounContext& prev, string docName);

	strIntProbMap _topicCounts;
	strIntProbMap _newTopicCounts;
	string _currentDoc;
	strToStr _dumpFileNames;
};

#endif
