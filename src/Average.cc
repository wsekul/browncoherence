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
		cerr<<"Average "<<CoherenceModel::FLAGS<<" [models]\n";
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

	MaxEntSelection& probs = model->_probs;
	NaiveMaxEntEGridT* modelT = dynamic_cast<NaiveMaxEntEGridT*>(model);

	if(!modelT)
	{
		int allDim = -1;
		for(int i = 2; i < argc; i++)
		{
			NaiveMaxEntEGrid* inModel = 
				dynamic_cast<NaiveMaxEntEGrid*>(
					CoherenceModel::loadFromFile(
						const_cast<char*>(flag.c_str()),
						argv[i]));

			int dim = inModel->_probs.dimension();
			if(allDim == -1)
			{
				allDim = dim;
				probs.setDimension(dim);
			}
			else
			{
				assert(allDim == dim);
			}
			for(int ii = 0; ii < dim; ++ii)
			{
				probs._weights[ii] += inModel->_probs._weights[ii];
			}
		}
	}
	else
	{
		IntVocab& vocab = modelT->_vocab;
		for(int i = 2; i < argc; i++)
		{
			NaiveMaxEntEGridT* inModel = 
				dynamic_cast<NaiveMaxEntEGridT*>(
					CoherenceModel::loadFromFile(
						const_cast<char*>(flag.c_str()),
						argv[i]));

			IntVocab& inVocab = inModel->_vocab;

			int dim = inModel->_probs.dimension();
			if(dim > probs.dimension())
			{
				probs.setDimension(dim);
			}

			for(int ii = 0; ii < dim; ++ii)
			{
				int featNum = inVocab.inv(ii);
				int myV = vocab.get(featNum, true);
				if(myV >= probs.dimension())
				{
					probs.setDimension(myV + 1);
				}

				probs._weights[myV] += inModel->_probs._weights[ii];
			}
		}
	}

	int norm = argc - 2;
	int dim = probs.dimension();
	for(int ii = 0; ii < dim; ++ii)
	{
//XXX this line was missing in early builds
		probs._weights[ii] /= norm;
	}

	model->write(cout);
}
