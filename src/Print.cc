#include "common.h"
#include "CoherenceModel.h"
#include "Document.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc != 3)
	{
		cerr<<"Print "<<CoherenceModel::FLAGS<<" [model]\n";
		cerr<<"Prints some model-specific debugging information.\n";
		abort();
	}

	CoherenceModel* model = 
		CoherenceModel::loadFromFile(argv[1], argv[2]);

	model->print(cout);
	delete model;
}
