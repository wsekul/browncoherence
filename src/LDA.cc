#include "LDA.h"

LDA::LDA(Vocab& voc, string& fpath, strings& docNames):
	_vocab(voc),
	_filePath(fpath)
{
	int ii = 0;
	foreach(strings, doc, docNames)
	{
		int sub = doc->rfind('/');
		string npart = doc->substr(sub + 1);
		_docToGamma[npart] = ii;

		int dash = npart.find('-');
		string secondNumber = npart.substr(dash + 1);
		if(secondNumber.size() == 1)
		{
			secondNumber = "0"+secondNumber;
		}
		string unslashed = npart.substr(0, dash) + secondNumber;

		if(unslashed.size() == 3)
		{
			string sourceDoc = "wsj_0"+unslashed+".mrg";

			_docToGamma[sourceDoc] = ii;
		}
		else
		{
			string sourceDoc = "wsj_"+unslashed+".mrg";

			_docToGamma[sourceDoc] = ii;
		}
		++ii;
	}
}

LDA::LDA(istream& is)
{
	read(is);
}

void LDA::write(ostream& os)
{
	_vocab.write(os);
	writeHashMap(_docToGamma, os);
	os<<_filePath<<"\n";
}

void LDA::read(istream& is)
{
	_vocab.read(is);
	readHashMap(_docToGamma, is);
	is>>_filePath;

	readData();
}

int LDA::topics()
{
	return _beta.size();
}

void LDA::projectWord(string wordStr, Probs& projection)
{
	int ntopics = topics();
//	cerr<<topics<<" TOPICS\n";
	Prob max = 1;

	projection.resize(ntopics);

	int word = _vocab.get(wordStr, false);
	if(word == -1)
	{
		for(int topic = 0; topic < ntopics; ++topic)
		{
			projection[topic] = 0;
		}
		return;
	}

//	cerr<<"index is "<<word<<"\n";

	//compute log P(word,topic)
	for(int topic = 0; topic < ntopics; ++topic)
	{
		assert(_beta.find(topic) != _beta.end());
		Prob bt = _beta[topic][word];
//		assert(bt < 0);

		Prob term = bt;

		assertLogProb(term);
		projection[topic] = term;
		if(max == 1 || term > max)
		{
			max = term;
		}
	}
	assert(max <= 0);

	//exponentiate
	Prob norm = 0;
	for(int topic = 0; topic < ntopics; ++topic)
	{
		projection[topic] = expl(projection[topic] - max);
		norm += projection[topic];
	}

	//normalize
	for(int topic = 0; topic < ntopics; ++topic)
	{
		projection[topic] /= norm;
	}
}

void LDA::readData()
{
	string betaFile = _filePath+"/final.beta";
//	string betaFile = _filePath+"/015.beta";
	cerr<<"Beta: "<<betaFile<<"\n";
	ifstream bfile(betaFile.c_str());
	assert(bfile.is_open());

	int ctr = 0;
	while(bfile)
	{
		string line;
		getline(bfile, line);
		std::istringstream is(line);
		Prob wp;
		Probs pr;

		while(is>>wp)
		{
			assert(wp < 0); //log probs
			pr.push_back(wp);
		}
		if(pr.size() > 0)
		{
			_beta[ctr] = pr;
		}
		++ctr;
	}

	string gammaFile = _filePath+"/final.gamma";
//	string gammaFile = _filePath+"/015.gamma";
	cerr<<"Gamma: "<<gammaFile<<"\n";
	ifstream gfile(gammaFile.c_str());
	assert(gfile.is_open());

	Prob alpha = .01; //XXX don't hardcode

	ctr = 0;
	Probs avgG;
	avgG.resize(topics());
	while(gfile)
	{
		string line;
		getline(gfile, line);
		std::istringstream is(line);
		Prob wp;
		Probs pr;
		Prob norm = 0;

		while(is>>wp)
		{
			assert(wp >= 0); //counts
			wp += alpha;
			pr.push_back(wp);
			norm += wp;
		}

		int size = pr.size();
		if(size > 0)
		{
			for(int ii = 0; ii < size; ++ii)
			{
				avgG[ii] += pr[ii];
				pr[ii] /= norm;
			}
			_gamma[ctr] = pr;
		}
		++ctr;
	}

	Prob n1 = 0;
	for(int ii = 0; ii < topics(); ++ii)
	{
		n1 += avgG[ii];
	}
	for(int ii = 0; ii < topics(); ++ii)
	{
		avgG[ii] /= n1;
	}

//	writeVector(avgG, cout);
}
