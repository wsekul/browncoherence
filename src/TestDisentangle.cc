#include "Disentangler.h"
#include "MultiwayDisentangler.h"
#include "common.h"
#include "CoherenceModel.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 4)
	{
		cerr<<"Disentangle "<<CoherenceModel::FLAGS<<
			" [model] [document]\n";
		cerr<<"Disentangles a two-dialogue transcript.\n";
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

	Disentangler* ins = NULL;
	if(gold->nThreads() < 2)
	{
		cerr<<"Can't disentangle a document without any threads!\n";
	}
	else if(gold->nThreads() == 2)
	{
		cerr<<"Selecting binary disentanglement...\n";
		ins = new Disentangler(model, gold, true);
	}
	else
	{
		cerr<<"Selecting multiway disentanglement...\n";
		ins = new MultiwayDisentangler(model, gold, true);
		cerr<<"True objective "<<ins->objective()<<"\n";

		static_cast<MultiwayDisentangler*>(ins)->greedyPartialSolve();
	}
	cerr<<"True objective "<<ins->objective()<<"\n";

	ins->tabooSolve();

	for(int ii = 0; ii < gold->size(); ++ii)
	{
		Sent* sent = (*gold)[ii];
		cout<<"T"<<(1 + sent->dialogue())<<" "<<sent->time()<<" "<<"S"<<
			sent->speaker()<<" :  "<<Sent::plaintext(sent->tree())<<"\n";
	}
}
