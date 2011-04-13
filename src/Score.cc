#include "common.h"
#include "CoherenceModel.h"
#include "Document.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 3)
	{
		cerr<<"Score "<<CoherenceModel::FLAGS<<
			" [model] [documents]\n";
		cerr<<"Prints scores of documents.\n";
		abort();
	}

	int nextArg = 1;
	CoherenceModel* model = 
		CoherenceModel::loadFromArgs(argv, argc, nextArg);

	for(int d = nextArg; d < argc; d++)
	{
		cerr<<"Reading file "<<argv[d]<<" ...\n";
		Document* doc = new Document(argv[d]);
		Prob score = model->logProbability(*doc);
		cout<<score<<"\n";
		delete doc;
	}

	delete model;
}
