#include "common.h"
#include "CoherenceModel.h"
#include "Document.h"
#include "Transcript.h"

#include "NaiveEGrid.h"
#include "NaiveMaxEntEGrid.h"
#include "NaiveMaxEntEGridT.h"
#include "PronounModel.h"
#include "PronounIBM.h"
#include "WordIBM.h"
#include "TimeModel.h"
#include "SpeakerModel.h"
#include "MentionModel.h"
#include "MixtureModel.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 2)
	{
		cerr<<"Train "<<CoherenceModel::FLAGS<<" [documents]\n";
		abort();
	}

	CoherenceModel* model = NULL;
	string flag(argv[1]); 	//yeah, this is a lame way to parse args...

	if(flag == "-n")
	{
		int hist = 2;
		int maxSal = 4;
		double smooth = 1;
		int gen = SLIGHTLY_GEN;

		cerr<<"Making egrid model with "<<hist<<" history items, "<<
			" max salience "<<maxSal<<", "<<
			" smoothing "<<smooth<<", "<<
			" generativity "<<gen<<".\n";
		model = new NaiveEGrid(hist, maxSal, smooth, gen);
	}
	else if(flag == "-m")
	{
		int hist = 6;
		int maxSal = 4;
		int gen = SLIGHTLY_GEN;

		cerr<<"Making max ent egrid model with "<<hist<<
			" history items, "<<
			" max salience "<<maxSal<<", "<<
			" generativity "<<gen<<".\n";
		model = new NaiveMaxEntEGrid(hist, maxSal, gen);
	}
	else if(flag == "-v")
	{
		int hist = 6;
		int maxSal = 4;
		int gen = SLIGHTLY_GEN;
		string lda = DATA_PATH + "/ldaSwbd/vocabNV";

		cerr<<"Making topic max ent egrid model with "<<hist<<
			" history items, "<<
			" max salience "<<maxSal<<", "<<
			" generativity "<<gen<<
			" lda "<<lda<<".\n";
		model = new NaiveMaxEntEGridT(hist, maxSal, gen, lda);
	}
	else if(flag == "-a")
	{
		cerr<<"Making address by name model.\n";
		model = new MentionModel();
	}
	else
	{
		cerr<<"Unrecognized flag "<<flag<<"\n";
		abort();
	}

	for(int i=2; i<argc; i++)
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

			Transcript trans(is, name);
			cerr<<" ... ";

			if(!trans.empty())
			{
				foreach(intDocumentMap, doc, trans.threads())
				{
					cerr<<doc->second->name()<<"...\n";
					model->train(*doc->second);
				}
			}
			cerr<<"\n";
			ctr++;
		}
	}

	cerr<<"Estimating parameters.\n";
	model->estimate();
	cerr<<"Done.\n";

	model->write(cout);

	delete model;
}
