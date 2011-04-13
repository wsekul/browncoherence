#include "common.h"
#include "CoherenceModel.h"
#include "Document.h"

#include "IBM.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 2)
	{
		cerr<<"TrainIBM "<<"-[w] (-[w] model)"<<"\n";
		abort();
	}

	string flag = argv[1];

	IBM* other = NULL;
	if(argc > 3)
	{
		string flag2(argv[2]);
		if(flag2[0] == '-')
		{
			cerr<<"Creating base model from "<<argv[3]<<"\n";
			ifstream infs (argv[3]);
			other = IBM::create(flag2, infs);
		}
	}

	IBM* model = NULL;
	if(other)
	{
		model = IBM::create(flag, other);
	}
	else
	{
		model = IBM::create(flag);
	}

	model->write(cout);
}
