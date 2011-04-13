#include "CoherenceModel.h"
#include "JointModel.h"

CoherenceModel::~CoherenceModel()
{
}

//you are allowed to print debugging info using this method
//it doesn't have to do anything
void CoherenceModel::print(ostream& os)
{

}

int CoherenceModel::history()
{
	return 1;
}

//main useful method of this model
//you don't have to write it, because it wraps more basic calls
//does init-cache / perm-prob (identity perm) / clear-cache / return
Prob CoherenceModel::logProbability(Document& doc)
{
	initCaches(doc);

	ints perm;
	for(int i=0; i<doc.size(); i++)
	{
		perm.push_back(i);
	}

	Prob res = permProbability(doc, perm, false);

	clearCaches();

	assert(isfinite(res));
	return res;
}

Probs& CoherenceModel::sentScores()
{
	return _sentScores;
}

Prob CoherenceModel::globalScore()
{
	assert(isfinite(_globalScore));
	return _globalScore;
}

//add a training document
//does init-cache / perm-prob (ident) true / clear-cache / return
void CoherenceModel::train(Document& doc)
{
	initCaches(doc);

	ints perm;
	for(int i=0; i<doc.size(); i++)
	{
		perm.push_back(i);
	}

	permProbability(doc, perm, true);

	clearCaches();
}

//the master loader... this is kind of lame, actually, because
//you have to keep updating the flag list to agree with train
//but it's way better than having a copy of this code in every
//driver file
CoherenceModel* CoherenceModel::loadFromFile(char* flag, char* file)
{
	ifstream is(file);
	if(!is.is_open())
	{
		cerr<<"Can't open the model "<<file<<"\n";
	}
	else
	{
		cerr<<"Model is: "<<file<<"\n";
	}

	return loadByFlag(flag, is);
}

CoherenceModel* CoherenceModel::loadFromArgs(char* args[], 
											 int argc, int& nextArg)
{
	JointModel* finalModel = NULL;
	CoherenceModel* firstModel = NULL;

	while(nextArg + 1 < argc)
	{
		string flag(args[nextArg]);

		if(flag[0] != '-' || flag == "--")
		{
			break;
		}

		//try to load a new model
		CoherenceModel* next = 
			loadFromFile(args[nextArg], args[nextArg+1]);

		//only construct a JointModel if we have to
		if(firstModel == NULL)
		{
			firstModel = next;
		}
		else if(finalModel == NULL)
		{
			finalModel = new JointModel();
			finalModel->addModel(firstModel);
			finalModel->addModel(next);

			firstModel = finalModel;
		}
		else
		{
			finalModel->addModel(next);
		}

		nextArg += 2;
	}

	return firstModel;
}

string CoherenceModel::FLAGS = "-[n|m|v|ww|wp|a|t|k|f|e|x]";

#include "NaiveEGrid.h"
#include "NaiveMaxEntEGrid.h"
#include "NaiveMaxEntEGridT.h"
#include "NaiveMaxEntEGridF.h"
#include "PronounModel.h"
#include "PronounIBM.h"
#include "WordIBM.h"
#include "TimeModel.h"
#include "SpeakerModel.h"
#include "MentionModel.h"
#include "DiscSyntaxConditional.h"
#include "MixtureModel.h"

CoherenceModel* CoherenceModel::loadByFlag(char* cflag, istream& is)
{
	CoherenceModel* model = NULL;

	string flag(cflag); //so I don't need strcmp

	if(flag == "-n")
	{
		cerr<<"Loading entity grid.\n";
		model = new NaiveEGrid(is);
	}
	else if(flag == "-m")
	{
		cerr<<"Loading max ent entity grid.\n";
		model = new NaiveMaxEntEGrid(is);
	}
	else if(flag == "-v")
	{
		cerr<<"Loading max ent entity grid with lda.\n";
		model = new NaiveMaxEntEGridT(is);
	}
	else if(flag == "-t")
	{
		cerr<<"Loading time gap model.\n";
		model = new TimeModel(is);
	}
	else if(flag == "-k")
	{
		cerr<<"Loading speaker model.\n";
		model = new SpeakerModel(is);
	}
	else if(flag == "-f")
	{
		cerr<<"Loading max ent naive entity grid with more features.\n";
		model = new NaiveMaxEntEGridF(is);
	}
	else if(flag == "-a")
	{
		cerr<<"Loading address by name model.\n";
		model = new MentionModel(is);
	}
	else if(flag == "-e")
	{
		cerr<<"Loading disc syntax conditional.\n";
		model = new DiscSyntaxConditional(is);
	}
	else if(flag[1] == 'w' || flag == "-ec")
	{
		return IBM::create(flag, is);
	}
	else if(flag == "-x")
	{
		cerr<<"Loading mixture model.\n";
		model = new MixtureModel(is);
	}
	else
	{
		cerr<<"Unrecognized flag "<<flag<<"\n";
		abort();
	}

	return model;
}
