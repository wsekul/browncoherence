#include "common.h"
#include "CoherenceModel.h"
#include "Document.h"

#include "IBM.h"
#include "TopicIBM.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 3)
	{
		cerr<<"CombineAndMax "<<"-[w]"<<
			" [counts]\n";
		abort();
	}

	string flag = argv[1];
	ifstream dump(argv[2]);
	assert(dump.is_open());

	IBM* model = NULL;
	model = IBM::create(flag, dump);

	TopicIBM* ti = dynamic_cast<TopicIBM*>(model);

	for(int i=3; i<argc; i++)
	{
		ifstream is(argv[i]);
		assert(is.is_open());
		cerr<<"Reading "<<argv[i]<<"...";

		if(ti)
		{
			ti->_currentDoc = string(argv[i]);
		}

		intIntMap vocabTrans;
		model->readCounts(is, vocabTrans);
		cerr<<"\n";
	}

	cerr<<"Estimating parameters.\n";
	model->smooth();
	model->normalize();
//	model->discardSmallParams();
	cerr<<"Done.\n";

	model->write(cout);
}
