#include "DialogueInserter.h"

DialogueInserter::DialogueInserter(CoherenceModel* model, Document& doc,
								   bool verbose):
	Inserter(model, doc, verbose)
{
}

int DialogueInserter::insert(int k)
{
	int n = _doc->segments();
	int turns = n/2;

	ints perm;

	double best = 0;
	ints bestPos;
	for(int pos = 0; pos < turns; pos++)
	{
		makePerm(k, pos, _doc, perm);

		if(_verbose)
		{
			cerr<<"\t"<<pos<<":\t";
//    			for(ints::iterator i = perm.begin(); i != perm.end(); i++)
//    			{
//    				cerr<<*i<<" ";
//    			}
		}

		double prob = _model->permProbability(*_doc, perm, false);
		
		if(_verbose)
		{
			double met = score(pos, k);
			cerr<<":\t"<<prob<<" "<<met<<"\n";
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

void DialogueInserter::makePerm(int move, int pos, Document* doc, ints& perm)
{
	perm.clear();

	ints& segs = doc->segmentStarts();
	int n = doc->segments();

	assert(move < n/2 && pos < n/2);

	int segMove = 2 * move;
	int justBefore = 2 * pos - 1;
	int printed = 0;

	for(int seg = 0; seg < n; seg++)
	{
		if(printed > justBefore) //insert the moved segment here
		{
			for(int i = segs[segMove];
				i < segs[segMove+2];
				i++)
			{
				perm.push_back(i);
			}
			justBefore = n; //don't print again
			printed += 2;
		}

		if(seg >= segMove && seg < segMove + 2)
		{
			//don't print the moved segment in its original pos
			continue;
		}

		++printed;
		//insert the next segment here
		for(int i = segs[seg];
			i < segs[seg+1];
			i++)
		{
			perm.push_back(i);
		}
	}

	if(printed != n)
	{
		for(int i = segs[segMove];
			i < segs[segMove+2];
			i++)
		{
			perm.push_back(i);
		}
	}
}

double DialogueInserter::score(int pos, int k)
{
	int possInserts = _doc->segments() / 2;

	return metric(pos, possInserts, k);
}
