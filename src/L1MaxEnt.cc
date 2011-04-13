#include "common.h"
#include "MaxEntSelection.h"
#include "Vocab.h"

int main(int argc, char* argv[])
{
	MaxEntSelection learner(2, 1.0);
	Vocab voc;
	voc.get("**BIAS**", true);

	ifstream file(argv[argc - 1]);
	string key;
	int nTrain = 0;
	while(file)
	{
		getline(file, key);
		std::istringstream readout(key);

		if(key == "DEV")
		{
			break;
		}

		int classN;
		readout>>classN;

		MaxEntContext ct;
		Feats neg;
		Feats pos;
		pos.push_back(Feat(0, 1));

		string fkey;
		Prob fv;
		while(readout>>fkey)
		{
			readout>>fv;

			int fKeyInd = voc.get(fkey, true);
			pos.push_back(Feat(fKeyInd, fv));
		}

		ct.push_back(neg);
		ct.push_back(pos);
		Prob score = 1.0;

		if(classN == 1)
		{
			score += .5;
		}

		learner.addCount(classN, ct, score);

		++nTrain;
	}
	Examples devs;
	while(file)
	{
		getline(file, key);
		std::istringstream readout(key);

		int classN;
		readout>>classN;

		MaxEntContext ct;
		Feats neg;
		Feats pos;
		pos.push_back(Feat(0, 1));

		string fkey;
		Prob fv;
		while(readout>>fkey)
		{
			readout>>fv;

			int fKeyInd = voc.get(fkey, true);
			pos.push_back(Feat(fKeyInd, fv));
		}

		ct.push_back(neg);
		ct.push_back(pos);
		Example* ex = new Example(ct);
		ex->counts[classN] += 1.0;
		devs.push_back(ex);
	}

	cerr<<"training "<<nTrain<<" dev "<<devs.size()<<"\n";

	learner.setDimension(voc.size());
	learner.estimate(true, 0, 0);

	if(devs.size())
	{
		int right = 0;
		foreach(Examples, devEx, devs)
		{
			Prob prob0 = learner.probToken(0, *(*devEx)->context);
			bool class0 = ((*devEx)->counts[0] > 0);
			if(class0 && prob0 > .5)
			{
				++right;
			}
			else if(!class0 && prob0 < .5)
			{
				++right;
			}
		}
		int wrong = devs.size() - right;
		cerr<<"Dev accuracy "<<right<<" "<<right/(double)devs.size()<<
			" err "<<wrong/(double)devs.size()<<"\n";
	}

	for(int ctr = 0; ctr < learner.dimension(); ++ctr)
	{
		if(learner.weights()[ctr] != 0)
		{
			cerr<<voc.inv(ctr)<<" "<<-learner.weights()[ctr]<<"\n";
		}
	}

	for(int ctr = 0; ctr < learner.dimension(); ++ctr)
	{
		cout<<voc.inv(ctr)<<" "<<-learner.weights()[ctr]<<"\n";
	}
}
