#include "common.h"
#include "CoherenceModel.h"
#include "Document.h"

#include "NaiveEGrid.h"
#include "NaiveMaxEntEGrid.h"
#include "NaiveMaxEntEGridT.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 2)
	{
		cerr<<"TrainFromFeatures "<<CoherenceModel::FLAGS<<" [features]\n";
		abort();
	}

	NaiveMaxEntEGrid* model = NULL;
	string flag(argv[1]); 	//yeah, this is a lame way to parse args...

	if(flag == "-m")
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

		string lda = DATA_PATH + "/ldaSwbd/vocabN";

		cerr<<"Making topic max ent egrid model with "<<hist<<
			" history items, "<<
			" max salience "<<maxSal<<", "<<
			" generativity "<<gen<<
			" lda "<<lda<<".\n";
		model = new NaiveMaxEntEGridT(hist, maxSal, gen, lda);
	}
	else
	{
		cerr<<"Unrecognized flag "<<flag<<"\n";
		abort();
	}

	for(int i = 2; i < argc; i++)
	{
		string fname(argv[i]);
		cerr<<"Reading from "<<fname<<" ... ";
		model->readFeatureDump(fname);
	}

	model->estimate();

	model->write(cout);

	delete model;
}
