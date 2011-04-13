#include "common.h"
#include "CoherenceModel.h"
#include "Document.h"
#include "popen.h"

#include "IBM.h"
#include "LDA.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	Vocab vocab;

	strings names;

	for(int i=1; i<argc; i++)
	{
		izstream is(argv[i]);
		int ctr = 0;

		while(!is.eof())
		{
			string name = argv[i];
			name += "-"+intToString(ctr);
			cerr<<name;

			Document doc(is, name);
			cerr<<" ... ";

			intToInt wCount;

			if(!doc.empty())
			{
				names.push_back(name);

//				foreach(NPSet, np, doc.nps())
// 				{
// 					int vind = vocab.get((*np)->head(), true);
// 					if(vind != -1)
// 					{
// 						wCount[vind] += 1;
// 					}
// 				}

				foreach(Document, ss, doc)
				{
					Trees leaves;
					Sent::getLeaves((*ss)->tree(), leaves);
					foreach(Trees, leaf, leaves)
					{
						char type = Sent::tag(*leaf)[0];
						if(type == 'N' || type == 'V')
						{
							string word = stem(*leaf);
							if(word[0] == '\'')
							{
								continue;
							}
							int vind = vocab.get(word, true);
							if(vind != -1)
							{
								wCount[vind] += 1;
							}
						}
					}
				}

				cout<<wCount.size()<<" ";
				foreach(intToInt, ii, wCount)
				{
					cout<<ii->first<<":"<<ii->second<<" ";
				}
				cout<<"\n";
			}

			cerr<<"\n";
			ctr++;
		}
	}

	string fname("[path-to-dir]/alpha1NV");
	LDA lda(vocab, fname, names);
	ofstream ofs("[path-to-dir]/vocabNV");
	lda.write(ofs);
}
