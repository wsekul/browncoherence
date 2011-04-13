#include <iomanip>

#include "Document.h"
#include "NaiveEGrid.h"
//#include "PronounNaiveEGrid.h"

void printGrid(const string& d, symToIntToInt& roles, int len);

int main(int argc, char* argv[])
{
	appInit(DATA_PATH);

	int start = 1;
	EGridModel* mod = NULL;
	if(string(argv[1]) == "-p")
	{
		//threshold 0 (last one) means accept all proposed referents
//		mod = new PronounNaiveEGrid(0, 0, 0, 0, 0, string("data/probs.txt"));
		start++;
	}
	else
	{
		mod = new NaiveEGrid(0, 0, 0, 0);
	}

	for(int i=start; i<argc; i++)
	{
		{
			ifstream check(argv[i]);
			assert(check.is_open());
		}

		izstream is(argv[i]);
		int ctr = 0;

		while(!is.eof())
		{
			string name = argv[i];
			if(ctr > 0)
			{
				name += "-"+intToString(ctr);
			}
			cerr<<name;

			Document* doc = new Document(is, name);
			cerr<<" ... ";

			if(!doc->empty())
			{
				cerr<<argv[i]<<"\n\n";

				mod->initCaches(*doc);
				printGrid(string(argv[i]), mod->roles(), doc->size());
				mod->clearCaches();
			}
		}
	}
}

void printGrid(const string& d, symToIntToInt& roles, int len)
{
	symSet printed;

	for(int i=0; i < len; i++)
	{
		for(symToIntToInt::iterator entity = roles.begin();
			entity != roles.end();
			entity++)
		{
			if(roles[entity->first][i] != T_NONE && 
			   !contains(printed, entity->first))
			{

//				cout<<d<<" ";
				cout<<setw(20)<<uc(SYMTAB->toString(entity->first));

				for(intToInt::iterator sym = entity->second.begin();
					sym != entity->second.end();
					sym++)
				{
					//cutoff
					if(sym->first > 28)
					{
						break;
					}

					cout<<" "<<NP::roleToString(sym->second);
				}
				cout<<"\n";

				printed.insert(entity->first);
			}
		}
	}
}
