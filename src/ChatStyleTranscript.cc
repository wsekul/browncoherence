#include "Disentangler.h"
#include "common.h"
#include "CoherenceModel.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 2)
	{
		cerr<<"ChatStyleTranscript "<<" [document]\n";
		cerr<<"Prints a two-dialogue transcript in chat format.\n";
		abort();
	}

	cerr<<"Reading the gold file "<<argv[1]<<" ...\n";
	Transcript* gold = new Transcript(argv[1]);

	for(int ii = 0; ii < gold->size(); ++ii)
	{
		Sent* sent = (*gold)[ii];
		cout<<"T"<<(1 + sent->dialogue())<<" "<<sent->time()<<" "<<"S"<<
			sent->speaker()<<" :  "<<Sent::plaintext(sent->tree())<<"\n";
	}
}
