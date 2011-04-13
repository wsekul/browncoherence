#include "common.h"
#include "CoherenceModel.h"
#include "Document.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 3)
	{
		cerr<<"Discriminate "<<CoherenceModel::FLAGS<<
			" [model] [gold] [test set]\n";
		cerr<<"Does binary discrimination of test documents vs. a "<<
			"gold document.\n";
		abort();
	}

	int nextArg = 1;
	CoherenceModel* model = 
		CoherenceModel::loadFromArgs(argv, argc, nextArg);

	double meanScore = 0;
	double margin = 0;

	cerr<<"Reading the gold file "<<argv[nextArg]<<" ...\n";
	Document* gold = new Document(argv[nextArg]);
	Prob goldScore = model->logProbability(*gold);
	nextArg++;

	meanScore += goldScore;
	cerr<<"Gold score: "<<goldScore<<"\n";

	int tests = 0;
	int wins = 0;
	int ties = 0;
	for(int d = nextArg; d < argc; d++)
	{
		tests++;

		cerr<<argv[d]<<"...\n";
		Document* other = new Document(argv[d]);
		Prob score = model->logProbability(*other);
		
		meanScore += score;
		margin += (goldScore - score);
		cerr<<"Score: "<<score;

		if(score < goldScore)
		{
			cerr<<" WIN\n";
			wins++;
		}
		else if(score == goldScore)
		{
			cerr<<" TIE\n";
			ties++;
		}
		else
		{
			cerr<<" LOSE\n";
		}

		delete other;
	}

	delete gold;

	cout<<tests<<" tests, "<<wins<<" wins, "<<ties<<" ties.\n";
	cerr<<"Mean score: "<<meanScore/(tests+1)<<".\n";
	cerr<<"Total score: "<<meanScore<<".\n";
	cerr<<"Average margin: "<<margin/tests<<".\n";

	delete model;
}
