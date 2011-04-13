#include "JointModel.h"

JointModel::JointModel()
{
}

JointModel::JointModel(istream& is)
{
	read(is);
}

JointModel::~JointModel()
{
	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++)
	{
		delete *i;
	}
}

//two serialization methods: should be inverses of each other

//serializes this class
void JointModel::write(ostream& os)
{
	cerr<<"Not implemented yet.\n";
	abort();
}

//initializes this class -- called by the istream constructor
void JointModel::read(istream& is)
{
	cerr<<"Not implemented yet.\n";
	abort();
}

//you are allowed to print debugging info using this method
//it doesn't have to do anything
void JointModel::print(ostream& os)
{
	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++)
	{
		(*i)->print(os);
	}
}

int JointModel::history()
{
	int max = 1;
	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++)
	{
		if(max < (*i)->history())
		{
			if((*i)->history() == GLOBAL_HIST)
			{
				continue;
			}

			max = (*i)->history();
		}
	}

	return max;
}

//cache info about a document
void JointModel::initCaches(Document& doc)
{
	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++)
	{
		(*i)->initCaches(doc);
	}
}

void JointModel::clearCaches()
{
	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++)
	{
		(*i)->clearCaches();
	}
}

//log probability of a permuted document
//or does training
Prob JointModel::permProbability(Document& doc, ints& perm, bool train)
{
	_sentScores.clear();
	_sentScores.resize(perm.size());
	_globalScore = 0;

	Prob res = 0.0;

	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++)
	{
		Prob term = (*i)->permProbability(doc, perm, train);
		//cerr<<"Term "<<term<<"\n";
		res += term;

		if((*i)->history() == GLOBAL_HIST)
		{
			_globalScore += term;
		}
	}

	int size = perm.size();
	foreach(CoherenceModels, model, _models)
	{
		for(int ii = 0; ii < size; ++ii)
		{
			_sentScores[ii] += (*model)->sentScores()[ii];
		}
	}

	return res;
}
	
//estimate parameters
void JointModel::estimate()
{
	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++)
	{
		(*i)->estimate();
	}
}

void JointModel::addModel(CoherenceModel* model)
{
	_models.push_back(model);
}

CoherenceModels& JointModel::models()
{
	return _models;
}
