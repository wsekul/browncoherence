#include "MixtureModel.h"
#include "common.h"
#include "CoherenceModel.h"
#include "DialogueInserter.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 4)
	{
		cerr<<"TrainMixture "<<CoherenceModel::FLAGS<<
			" [models] [document]\n";
		cerr<<"Trains mixture for disentanglement.\n";
		abort();
	}

	int nextArg = 1;
	MixtureModel* model = new MixtureModel();
	while(nextArg + 1 < argc)
	{
		if(argv[nextArg][0] != '-')
		{
			break;
		}

		string flag(argv[nextArg]);
		string file(argv[nextArg + 1]);
		model->add(flag, file);

		nextArg += 2;
	}

	for(int i=nextArg; i<argc; i++)
	{
		{
			ifstream check(argv[i]);
			assert(check.is_open());
		}

		izstream is(argv[i]);
		int ctr = 0;

		while(!is.eof())
		{
			string name = argv[i];
			if(ctr > 0)
			{
				name += "-"+intToString(ctr);
			}
			cerr<<name;
			++ctr;

			Document gold(is, name);

			if(ctr % 5 == 0)
			{
				cerr<<"...\n";
				int n = gold.size();
				for(int test = 0; test < 1; ++test)
				{
					ints badPerm;
					randPermutation(badPerm, n);
					model->recordDiscrimination(gold, badPerm);
				}
			}
			else
			{
				cerr<<"... skipped\n";
			}
		}
	}

	model->estimate();
	model->write(cout);
}
