#include "common.h"
#include "CoherenceModel.h"
#include "Document.h"

double fscore(int wins,int ties,int tests);

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 3)
	{
		cerr<<"DiscriminateRand "<<CoherenceModel::FLAGS<<
			" [model] [gold files]\n";
		cerr<<"Does binary discrimination of each gold document vs.  "<<
			"a random permutation.\n";
		abort();
	}

	int nextArg = 1;
	CoherenceModel* model = 
		CoherenceModel::loadFromArgs(argv, argc, nextArg);
	assert(model);

	int perms = 20; //sets number of permutations to use

	double meanScore = 0;
	double meanGold = 0;
	double margin = 0;
	int tests = 0;
	int wins = 0;
	int ties = 0;

	ints perm;

	for(int i=nextArg; i<argc; i++)
	{
		ifstream is(argv[i]);
		int ctr = 0;

		while(!is.eof())
		{
			string name = argv[i];
			if(ctr > 0)
			{
				name += "-"+intToString(ctr);
			}

			Document* doc = new Document(is, name);

			if(doc->size() > 1)
			{
				cerr<<name<<" ...\n";
				ctr++;

				Prob goldScore = model->logProbability(*doc);
				meanScore += goldScore;
				meanGold += goldScore;
				cerr<<"Gold score: "<<goldScore<<"\n";

				tests++;

				//with this line active, does the same thing as
				//running DiscriminateRand separately per document
				//as the distributor usually does
				srand(0);

				for(int p = 0; p < perms; p++)
				{
					model->initCaches(*doc);
					randPermutation(perm, doc->size());
					Prob score = model->permProbability(*doc, perm, false);
					model->clearCaches();

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
				}
			}

			delete doc;
		}
	}

	int totTests = perms*tests;

	cout<<totTests<<" tests, "<<wins<<" wins, "<<ties<<" ties.\n";
	cerr<<"Mean score: "<<meanScore/(tests + totTests)<<".\n";
	cerr<<"Total score: "<<meanScore<<".\n";
	cerr<<"Mean gold score: "<<meanGold/tests<<".\n";
	cerr<<"Total gold score: "<<meanGold<<".\n";
	cerr<<"Average margin: "<<margin/tests<<".\n";
	cerr<<"Accuracy: "<<(wins/(double)totTests)<<" F: "<<
		fscore(wins, ties, totTests)<<".\n";

	delete model;
}

double fscore(int wins,int ties,int tests)
{
	double win =(double)wins;
	double loss=(double)(tests-(wins+ties));
	double prec = 0.0;
	if((win+loss) > 0)
	{
		prec = win/(win+loss);
	}
	double recall =win/(double)tests;
	if(prec+recall > 0)
	{
		return (2*prec*recall)/(prec+recall);
	}
	return 0;
}
