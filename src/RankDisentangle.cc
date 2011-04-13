#include "Disentangler.h"
#include "MultiwayDisentangler.h"
#include "common.h"
#include "CoherenceModel.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 4)
	{
		cerr<<"RankDisentangle "<<CoherenceModel::FLAGS<<
			" [model] [document]\n";
		cerr<<"Prints rank of true disentanglement.\n";
		abort();
	}

	int nextArg = 1;
	CoherenceModel* model = 
		CoherenceModel::loadFromArgs(argv, argc, nextArg);

	cerr<<"Reading the gold file "<<argv[nextArg]<<" ...\n";
	Transcript* gold = new Transcript(argv[nextArg]);

// 	cerr<<"000000000000000000000000000000\n"<<gold->thread(0)<<"\n";
// 	cerr<<"111111111111111111111111111111\n"<<gold->thread(1)<<"\n";
// 	cerr<<"\n";

	intSet really0;
	foreach(Document, sent, gold->thread(0))
	{
		really0.insert((*sent)->index());
	}

	Disentangler* ins = NULL;
	if(gold->nThreads() < 2)
	{
		cerr<<"Can't disentangle a document without any threads!\n";
	}
	else if(gold->nThreads() == 2)
	{
		ins = new Disentangler(model, gold, true);
	}
	else
	{
		ins = new MultiwayDisentangler(model, gold, true);
	}

	cerr<<"True objective "<<ins->objective()<<"\n";

	cout<<"Rank "<<ins->approxRank()<<"\n";
//	cout<<"Rank "<<ins->rank()<<"\n";
}
