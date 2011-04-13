#include "common.h"
#include "CoherenceModel.h"
#include "Document.h"

#include "IBM.h"
#include "TopicIBM.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 5)
	{
		cerr<<"DistributedExpectations "<<"-[w] (model|CONSTRUCT)"<<
			" [start doc] [num docs] [documents]\n";
		abort();
	}

	IBM* model = NULL;
	string flag = argv[1];
	ifstream dump(argv[2]);
	assert(dump.is_open());

	int start = 3;

	model = IBM::create(flag, dump);

// 	if(endswith(string(argv[2]), "setup-0"))
// 	{
// 		model->setEstimating(false);
// 	}
// 	else
// 	{
// 		model->setEstimating(true);
// 	}

	int startDoc = atoi(argv[start]);
	int read = atoi(argv[start+1]);

	int docs = 0;
	Prob ll = 0.0;
	for(int i=start+2; i<argc; i++)
	{
		{
			ifstream check(argv[i]);
			assert(check.is_open());
		}

		izstream is(argv[i]);

		int ctr = 0;

		while(!is.eof())
		{
			if(docs < startDoc)
			{
				skipDocument(is);
			}
			else
			{
				string name = argv[i];
				if(ctr > 0)
				{
					name += "-"+intToString(ctr);
				}

				cerr<<name;

				Document doc(is, name);
				cerr<<" ... ";

				if(!doc.empty())
				{
					ll += model->review(doc);
				}
				cerr<<"\n";
			}
			ctr++;
			docs++;

			if(docs == startDoc + read)
			{
				i = argc + 1;
				break;
			}
		}
	}

	cerr<<"Likelihood: "<<ll<<"\n";
	model->writeCounts(cout);
}
