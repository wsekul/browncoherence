#include "DialogueInserter.h"
#include "common.h"
#include "CoherenceModel.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 4)
	{
		cerr<<"DialogueInsert "<<CoherenceModel::FLAGS<<
			" [model] [document]\n";
		cerr<<"Does block insertion on each turn of a dialogue.\n";
		abort();
	}

	int nextArg = 1;
	CoherenceModel* model = 
		CoherenceModel::loadFromArgs(argv, argc, nextArg);

	cerr<<"Reading the gold file "<<argv[nextArg]<<" ...\n";
	Document* gold = new Document(argv[nextArg]);

	DialogueInserter ins(model, *gold, true);

	cerr<<*gold<<"\n\n";

	ints segs = gold->segmentStarts();

	double meanScore = 0;
	int perfects = 0;
	int tests = 0;
	int turns = gold->segments() / 2;

	for(int k=0; k<turns;k++)
	{
		for(int segPart = segs[2 * k];
			segPart < segs[2 * k + 2];
			segPart++)
		{
			cerr<<*(*gold)[segPart]<<"\n";
		}
		cerr<<"\n";

		int pos = ins.insert(k);
		double score = ins.score(pos, k);
		cerr<<"Removed: "<<k<<"\tInserted: "<<pos<<"\n";
		cerr<<"Score: "<<score<<"\n";

		if(!isfinite(score))
		{
			cerr<<"Skipping, only one possible insertion point.\n";
			continue;
		}

		tests++;
		meanScore += score;

		if(k == pos)
		{
			perfects++;
		}
	}

	meanScore /= tests;
	cout<<"Mean: "<<meanScore<<"\n";
	cout<<"Perfect: "<<perfects<<" of "<<tests<<"\n";
}
