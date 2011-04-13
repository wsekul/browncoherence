#include "common.h"
#include "CoherenceModel.h"
#include "Document.h"
#include "popen.h"

#include "IBM.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 2)
	{
		cerr<<"TransferIBM "<<"-[w]"<<" [documents]\n";
		abort();
	}

	IBM* model = NULL;
	string flag(argv[1]); 	//yeah, this is a lame way to parse args...

	IBM* other = NULL;
	string flag2(argv[2]);
	if(flag2[0] == '-')
	{
		cerr<<"Creating base model from "<<argv[3]<<"\n";
		ifstream infs (argv[3]);
		other = IBM::create(flag2, infs);
	}
	assert(other);

	ifstream infs1(argv[3]);
	model = IBM::create(flag, other);

	for(int i=4; i<argc; i++)
	{
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

			Document doc(is, name);
			cerr<<" ... ";

			if(!doc.empty())
			{
				model->initCaches(doc);
				ints perm;
				for(int i=0; i<doc.size(); i++)
				{
					perm.push_back(i);
				}
				model->emTransfer(doc, perm, true, other);
				model->clearCaches();
			}

			cerr<<"\n";
			ctr++;
		}
	}

	cerr<<"Estimating parameters.\n";
	model->setEstimating(true);
	model->smooth();
	model->normalize();

	for(int iter=0; iter < 5; iter++)
	{
//		model->print(cerr);

		Prob ll = 0.0;
		int docs = 0;

		cerr<<"EM: "<<iter<<"\n";
		for(int i=4; i<argc; i++)
		{
			izstream is(argv[i]);
			int ctr = 0;

			while(!is.eof())
			{
				if(docs % 100 == 0)
				{
					cerr<<"\t"<<docs<<"\n";
				}

				string name = argv[i];
				if(ctr > 0)
				{
					name += "-"+intToString(ctr);
				}
				Document doc(is, name);

				if(!doc.empty())
				{
					ll += model->review(doc);
					docs++;
				}
				ctr++;
			}
		}

		cerr<<"Likelihood: "<<ll<<"\n";

		model->smooth();
		model->normalize();
	}

	cerr<<"Done.\n";

	model->write(cout);

	delete model;
}
