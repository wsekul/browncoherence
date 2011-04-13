#include "Document.h"

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
			Document doc(is, name);

			if(!doc.empty())
			{
				ctr++;

				cout<<doc<<"\n";
			}
		}
	}
}
