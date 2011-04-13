#include "Inserter.h"

Inserter::Inserter(CoherenceModel* model, Document& doc, bool verbose):
	_model(model),
	_doc(&doc),
	_verbose(verbose)
{
	_model->initCaches(doc);
}

Inserter::~Inserter()
{
	_model->clearCaches();
}

int Inserter::insert(int k)
{
	int n = _doc->size();

	ints perm;
	perm.push_back(k);
	for(int i=0; i<n; i++)
	{
		if(i != k)
		{
			perm.push_back(i);
		}
	}

	double best = 0;
	ints bestPos;
	for(int pos=0; pos < n; pos++)
	{
		if(_verbose)
		{
			cerr<<"\t"<<pos<<":\t";
// 			for(ints::iterator i = perm.begin(); i != perm.end(); i++)
// 			{
// 				cerr<<*i<<" ";
// 			}
		}

		double prob = _model->permProbability(*_doc, perm, false);
		
		if(_verbose)
		{
			double met = metric(pos, n, k);
			cerr<<prob<<":\t"<<met<<"\n";
		}

		if(bestPos.empty() || prob > best)
		{
			best = prob;
			bestPos.clear();
			bestPos.push_back(pos);
		}
		else if(prob == best)
		{
			bestPos.push_back(pos);
		}

		if(pos < n-1)
		{
			perm[pos] = perm[pos+1];
			perm[pos+1] = k;
		}
	}

	assert(!bestPos.empty());
	if(bestPos.size() == 1)
	{
		return bestPos[0];
	}
	else
	{
		int toReturn = random() % bestPos.size();
		return bestPos[toReturn];
	}
}

double Inserter::score(int pos, int k)
{
	return metric(pos, _doc->size(), k);
}

double Inserter::metric(int pos, int n, int k)
{
	int onePos = pos+1; //from 0-indexed to 1-indexed
	int oneK = k+1;
	assert(onePos >= 1 && onePos <= n);
	return 1 - abs(oneK-onePos)/norm(n,k);
}

double Inserter::norm(int n, int k)
{
	int oneK = k+1;
	assert(oneK >= 1 && oneK <= n);
	return 1/(2.0*n) * (oneK*(oneK-1) + (n-oneK+1)*(n-oneK));
}
