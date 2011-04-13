#include "DiscSyntaxConditional.h"

DiscSyntaxConditional::DiscSyntaxConditional(bool byRef, bool useNewHead):
	NewEntityModel(0, 0, byRef, useNewHead),
	_probs(0)
{
}

DiscSyntaxConditional::DiscSyntaxConditional(istream& is):
	NewEntityModel(0, 0, false, false),
	_probs(0)
{
	read(is);
}

DiscSyntaxConditional::~DiscSyntaxConditional()
{
}

void DiscSyntaxConditional::init()
{
}

void DiscSyntaxConditional::write(ostream& os)
{
	os<<"DISC_SYNTAX_CONDITIONAL\n";
	os<<_byRef<<"\t"<<_newHead<<"\n";
	_probs.write(os);
	_feats.write(os);
}

void DiscSyntaxConditional::read(istream& is)
{
	string key;
	is>>key;
	assert(key == "DISC_SYNTAX_CONDITIONAL");
	is>>_byRef>>_newHead;
	_probs.read(is);
	_feats.read(is);

	init();
}

void DiscSyntaxConditional::estimate()
{
	_probs.setDimension(_feats.size());
	_probs.estimate();
}

Prob DiscSyntaxConditional::npProb(NP* np, int label, bool isNewHead,
								   int occurrences, int sent, bool train)
{
//	cerr<<Sent::plaintext(np->node())<<"\n";

	strToProb feats;

	//length
	Trees leaves;
	Sent::getLeaves(np->node(), leaves);
	int pre = 0;
	int post = 0;
	bool head = false;
	for(Trees::iterator i = leaves.begin();
		i != leaves.end();
		i++)
	{
		if(*i == Sent::head(np->node()))
		{
			head = true;
		}
		else if(!head)
		{
			pre++;
		}
		else
		{
			post++;
		}
	}

	feats["BIAS"] = 1;

// 	cerr<<"LENGTH "<<(pre+post+1)<<"\n";
// 	cerr<<"PRELENGTH "<<pre<<"\n";
// 	cerr<<"POSTLENGTH "<<post<<"\n";
	feats["LENGTH"] = (pre+post+1);
	feats["PRELENGTH"] = pre;
	feats["POSTLENGTH"] = post;

	//modifiers
	int depth = 0;
	Tree* level = np->node();
	int preterm = 0;

	while(preterm < 2)
	{
		depth++;
//		cerr<<Sent::plaintext(level)<<"\n";

		if(Sent::preterm(level))
		{
//			cerr<<"HEAD_"<<Sent::tag(level)<<"\n";
			feats["HEAD_"+Sent::tag(level)] = 1;
		}
		else
		{
			bool head = false;
			bool first = true;
			for(Tree* t = level->subtrees;
				t != NULL;
				t = t->sibling)
			{
				if(first && Sent::preterm(t))
				{
					string word = Sent::word(t->subtrees);
					if(Sent::proper(t->label) && word[word.length()-1] == '.')
					{
//						cerr<<"ABBR_FIRST"<<"\n";
						feats["ABBR_FIRST"] = 1;
					}
				}
				first = false;

				if(t->sibling == NULL && Sent::preterm(t))
				{
					string word = Sent::word(t->subtrees);
					if(Sent::proper(t->label) && word[word.length()-1] == '.')
					{
//						cerr<<"ABBR_LAST"<<"\n";
						feats["ABBR_LAST"] = 1;
					}
				}

				if(t->label == dtLabel)
				{
					if(Sent::word(t->subtrees) == "the")
					{
//						cerr<<"DET_THE"<<"\n";
						feats["DET_THE"] = 1;
					}
					else if(Sent::word(t->subtrees) == "a" ||
							Sent::word(t->subtrees) == "an")
					{
//						cerr<<"DET_A"<<"\n";
						feats["DET_A"] = 1;
					}
				}

				if(t != Sent::head(level))
				{
					if(!head)
					{
//						cerr<<"PREMOD_"<<Sent::tag(t)<<"\n";
						feats["PREMOD_"+Sent::tag(t)]++;
					}
					else
					{
//						cerr<<"POSTMOD_"<<Sent::tag(t)<<"\n";
						feats["POSTMOD_"+Sent::tag(t)]++;
					}

// 					if(Sent::appositive(t))
// 					{
// 						feats["APPOSITIVE"] = 1;
// 					}
				}
				else
				{
					head = true;
				}
			}
		}

		level = Sent::head(level);

		if(Sent::preterm(level))
		{
			preterm++;
		}
	}
//	cerr<<"DEPTH_"<<intToString(depth)<<"\n";
//	cerr<<"\n";
	feats["DEPTH_"+intToString(depth)] = 1;

	if(_newHead)
	{
		feats["NEW_HEAD"] = isNewHead;
	}

	if(train)
	{
		if(!_newHead && label == DISC_SINGLE)
		{
			return log(0);
		}

		MaxEntContext context;
		Feats neg;
		Feats pos;
		foreach(strToProb, ft, feats)
		{
			int ftInd = _feats.get(ft->first, train);
			if(ftInd != -1)
			{
				pos.push_back(Feat(ftInd, ft->second));
			}
		}
		context.push_back(neg);
		context.push_back(pos);

		if(label == DISC_OLD)
		{
			_probs.addCount(0, context);
		}
		else
		{
			_probs.addCount(1, context);
		}

		return log(0);
	}
	else
	{
		MaxEntContext context;
		Feats neg;
		Feats pos;
		foreach(strToProb, ft, feats)
		{
			int ftInd = _feats.get(ft->first, train);
			if(ftInd != -1)
			{
				pos.push_back(Feat(ftInd, ft->second));
			}
		}
		context.push_back(neg);
		context.push_back(pos);

		Prob prob = _probs.probToken(1, context);

		if(label == DISC_OLD)
		{
			prob = 1 - prob;
		}

		return log(prob);
	}
}
