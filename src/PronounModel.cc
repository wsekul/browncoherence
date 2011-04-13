#include "PronounModel.h"
#include "stripTree.h"

PronounModel::PronounModel(string& filename):
	_pron(filename, 0.0)
{
}

PronounModel::PronounModel(istream& in)
{
	read(in);
}

PronounModel::~PronounModel()
{
}

//two serialization methods: should be inverses of each other

//serializes this class
void PronounModel::write(ostream& os)
{
	os<<"PRONOUN_MODEL\n";
	_pron.write(os);
}

//initializes this class -- called by the istream constructor
void PronounModel::read(istream& is)
{
	string key;
	is>>key;
	assert(key == "PRONOUN_MODEL");

	_pron.read(is);
}

//cache info about a document
void PronounModel::initCaches(Document& doc)
{
	_pron.initCaches(doc);
}

void PronounModel::clearCaches()
{
	_pron.clearCaches();
}

//log probability of a permuted document
//or does training
Prob PronounModel::permProbability(Document& doc, ints& perm, bool train)
{
	if(train)
	{
		cerr<<"Warning: doesn't do training.\n";
		return log(0.0);
	}

	return _pron.permProbability(doc, perm);
}

//estimate parameters
void PronounModel::estimate()
{
	cerr<<"Warning: does nothing!\n";
}
