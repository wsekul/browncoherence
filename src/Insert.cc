#include "Inserter.h"
#include "common.h"
#include "CoherenceModel.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	if(argc < 4)
	{
		cerr<<"Insert "<<CoherenceModel::FLAGS<<
			" [model] [document]\n";
		cerr<<"Does sentence insertion on each sentence of a document.\n";
		abort();
	}

	int nextArg = 1;
	CoherenceModel* model = 
		CoherenceModel::loadFromArgs(argv, argc, nextArg);

	int totalIns = 0;
	int totalPerf = 0;
	double docAvgPerf = 0;

	double docAvgPos = 0;
	double lineAvgPos = 0;

	int docs = 0;

	for(int i=nextArg; i<argc; i++)
	{
		ifstream is(argv[i]);
		int ctr = 0;

		while(!is.eof())
		{
			string name = argv[i];
			if(ctr > 0)
			{
				name += "-"+intToString(ctr);
			}

			Document* doc = new Document(is, name);

			if(doc->size() > 1)
			{
				cerr<<name<<" ...\n";
				ctr++;
				docs++;

				Inserter ins(model, *doc, true);
				cerr<<*doc<<"\n\n";

				int perfects = 0;
				double docMean = 0;
				for(int k=0; k<doc->size();k++)
				{
					cerr<<*(*doc)[k]<<"\n";
					int pos = ins.insert(k);
					double score = ins.score(pos, k);
					cerr<<"Removed: "<<k<<"\tInserted: "<<pos<<"\n";
					cerr<<"Score: "<<score<<"\n";
					docMean += score;

					if(k == pos)
					{
						perfects++;
					}
				}

				totalPerf += perfects;
				totalIns += doc->size();
				docAvgPerf += (perfects / (double)doc->size());

				lineAvgPos += docMean;
				docAvgPos += docMean / (double)doc->size();

				cerr<<"Document mean: "<<(docMean / (double)doc->size())<<"\n";
				cerr<<"Document perfect: "<<
					perfects<<" of "<<doc->size()<<"\n";
			}

			delete doc;
		}
	}

	cerr<<"\n";
	cout<<"Mean: "<<docAvgPos<<"\n";
	cout<<"Perfect: "<<totalPerf<<" of "<<totalIns<<"\n";
	cerr<<"Perfect (by line): "<<(totalPerf / (double)totalIns)<<"\n";
	cerr<<"Perfect (by doc): "<<(docAvgPerf / (double)docs)<<"\n";
	cerr<<"Mean (by line): "<<(lineAvgPos / (double)totalIns)<<"\n";
}
