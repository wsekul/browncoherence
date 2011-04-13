#include "Transcript.h"

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	for(int i=1; i<argc; i++)
	{
		izstream is(argv[i]);
		int ctr = 0;

		while(!is.eof())
		{
			string name = argv[i];
			if(ctr > 0)
			{
				name += intToString(ctr);
			}
			cout<<name<<"\n\n";
			Transcript doc(is, name);

			if(!doc.empty())
			{
				ctr++;

				cout<<doc<<"\n";

//				cout<<doc.thread(0)<<"\n";

// 				cout<<"---------------------\n\n";

// 				for(int i = 0; i < doc.size(); ++i)
// 				{
// 					if(doc[i]->dialogue() == 0)
// 					{
// 						doc.take(i);
// 						doc.put(i, 1);
// 					}
// 				}

				for(int t = 0; t < doc.nThreads(); ++t)
				{
					cout<<"T"<<t<<" ********************\n\n";
					cout<<doc.thread(t)<<"\n";
				}
			}
		}
	}
}
