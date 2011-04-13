#include "MixtureModel.h"
#include "MultiwayDisentangler.h"
#include "DialogueInserter.h"

MixtureModel::MixtureModel():
	JointModel(),
	_probs(0)
{
}

MixtureModel::MixtureModel(istream& is):
	JointModel(),
	_probs(0)
{
	read(is);
}

void MixtureModel::write(ostream& os)
{
	os<<"MIXTURE\n";
	foreach(strings, arg, _args)
	{
		os<<*arg<<" ";
	}
	os<<" -- \n";
	_probs.write(os);
}

void MixtureModel::read(istream& is)
{
	checkToken(is, "MIXTURE");
	string nextArg;
	while(true)
	{
		is>>nextArg;
		if(nextArg == "--")
		{
			break;
		}
		string nextFile;
		is>>nextFile;
		add(nextArg, nextFile);
	}
	_probs.read(is);
}

// void MixtureModel::recordPos(Transcript& transcript)
// {
// 	_posFeats.clear();

// 	int ctr = 0;
// 	for(CoherenceModels::iterator i = _models.begin();
// 		i != _models.end();
// 		i++, ctr++)
// 	{
// 		Prob term = 0;
// 		foreach(Documents, thread, transcript.threads())
// 		{
// 			term += (*i)->logProbability(**thread);
// 		}

// //		cerr<<"Pos term "<<term<<"\n";
// 		_posFeats.push_back(Feat(ctr, term));
// 	}
// }

// Prob MixtureModel::recordNeg(Transcript& transcript, bool train)
// {
// 	MaxEntContext context;
// 	Feats neg;
// 	Feats pos;

// 	int ctr = 0;
// 	for(CoherenceModels::iterator i = _models.begin();
// 		i != _models.end();
// 		i++, ctr++)
// 	{
// 		Prob term = 0;
// 		foreach(Documents, thread, transcript.threads())
// 		{
// 			term += (*i)->logProbability(**thread);
// 		}

// 		Feat& posF = _posFeats[ctr];
// 		Prob diff = posF.second - term;
// 		pos.push_back(Feat(ctr, diff));
// 	}

// 	context.push_back(pos);
// 	context.push_back(neg);

// 	if(train)
// 	{
// 		_probs.addCount(0, context);
// 		return log(0);
// 	}
// 	else
// 	{
// // 		cerr<<_probs.weight(_posFeats)<<" ";
// // 		cerr<<_probs.weight(neg)<<" ";
// // 		cerr<<_probs.probToken(0, context)<<"\n";

// 		return _probs.probToken(0, context);
// 	}
// }

void MixtureModel::initTrain(Transcript& transcript)
{
	foreach(Disentanglers, di, _disentanglers)
	{
		delete *di;
	}
	_disentanglers.clear();

	bool multiway = transcript.nThreads() > 2;

	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++)
	{
		if(!multiway)
		{
			_disentanglers.push_back(new Disentangler(*i, &transcript, true));
		}
		else
		{
			_disentanglers.push_back(
				new MultiwayDisentangler(*i, &transcript, true));
		}
		Disentangler* last = _disentanglers.back();
		last->evaluateMoves();
	}
}

Prob MixtureModel::recordDual(Transcript& transcript, int sent, bool train,
							  int bestState, Prob score)
{
	MaxEntContext context;
	Feats neg;
	Feats pos;

	int ctr = 0;
	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++, ctr++)
	{
		Disentangler& dis = *_disentanglers[ctr];

		int trueThread = transcript[sent]->dialogue();
		bool singleton = transcript.thread(trueThread).empty();

		foreach(intDocumentMap, thread, transcript.threads())
		{
			int target = thread->first;

			if(target == trueThread)
			{
				continue;
			}
			if(transcript.thread(target).empty() && singleton)
			{
				continue;
			}
// 			//XXX not actually cheating but supports a cheat
// 			if(transcript.thread(target).empty())
// 			{
// 				continue;
// 			}

			Prob term = dis._moves[sent][target];
//			cerr<<"Gain for "<<ctr<<" "<<term<<"\n";
			pos.push_back(Feat(ctr, term));
		}
	}

	context.push_back(pos);
	context.push_back(neg);

	if(train)
	{
		_probs.addCount(bestState, context, score);
		return log(0);
	}
	else
	{
// 		cerr<<_probs.weight(neg)<<" ";
// 		cerr<<_probs.probToken(0, context)<<"\n";

		return _probs.probToken(bestState, context);
	}
}

Prob MixtureModel::record(Transcript& transcript, int sent, bool train,
						  int bestState, Prob score)
{
	if(transcript.nThreads() == 2)
	{
		//cerr<<"Activating dual mode.\n";
		return recordDual(transcript, sent, train, bestState, score);
	}
	else
	{
		//not bothering to build in support for uncertainty about the
		//best state... could be done though
		assert(bestState == 0);
	}

	MaxEntContext context;

	foreach(intDocumentMap, thread, transcript.threads())
	{
		Feats fts;
		context.push_back(fts);
	}

	int trueThread = transcript[sent]->dialogue();
	bool singleton = transcript.thread(trueThread).empty();

	int ctr = 0;
	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++, ctr++)
	{
		Disentangler& dis = *_disentanglers[ctr];

		Prob realScore = (*i)->logProbability(transcript.thread(trueThread));

		foreach(intDocumentMap, thread, transcript.threads())
		{
			int target = thread->first;

			if(transcript.thread(target).empty() && singleton)
			{
				continue;
			}

			Prob term = dis._moves[sent][target] + realScore;

			context[target].push_back(Feat(ctr, -term));
		}
	}

	if(train)
	{
		_probs.addCount(trueThread, context, score);
		return log(0);
	}
	else
	{
// 		cerr<<_probs.weight(neg)<<" ";
// 		cerr<<_probs.probToken(0, context)<<"\n";

		return _probs.probToken(trueThread, context);
	}
}

Prob MixtureModel::recordInsertion(Document& doc, int sent)
{
	_probs.setDimension(_models.size());
	for(int mi = 0; mi < _models.size(); ++mi)
	{
		_probs.weights()[mi] = 1;
	}

	MaxEntContext context;
	Feats neg;
	Feats pos;

	DialogueInserter ins(this, doc, false);

	int targ = ins.insert(sent);
	if(targ == sent)
	{
		return log(0);
	}

	Prob score = ins.score(targ, sent);
	score += 1;
	if(score < 0)
	{
		score = 0;
	}
	score /= 2;

	ints perm;
	ins.makePerm(sent, targ, &doc, perm);

	cerr<<"True: "<<sent<<" Target: "<<targ<<" score "<<score<<"\n";

	int ctr = 0;
	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++, ctr++)
	{
		Prob correct = (*i)->logProbability(doc);
		(*i)->initCaches(doc);
		Prob wrong = (*i)->permProbability(doc, perm, false);
		(*i)->clearCaches();
		Prob term = wrong - correct;
//		cerr<<"\t "<<_args[ctr * 2]<<" term "<<term<<"\n";
		pos.push_back(Feat(ctr, term));
	}

	context.push_back(pos);
	context.push_back(neg);

	_probs.addCount(0, context, score);
	return log(0);
}

Prob MixtureModel::recordDiscrimination(Document& doc, ints& badPerm)
{
	Probs scores;
	Probs badScores;
	int ctr = 0;
	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++, ctr++)
	{
		Prob correct = (*i)->logProbability(doc);
		scores.push_back(correct);
		cerr<<". ";
	}

	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++, ctr++)
	{
		(*i)->initCaches(doc);
		Prob wrong = (*i)->permProbability(doc, badPerm, false);
		(*i)->clearCaches();
		badScores.push_back(wrong);
		cerr<<". ";
	}
	cerr<<"\n";

	MaxEntContext context;
	Feats neg;
	Feats pos;

	ctr = 0;
	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++, ctr++)
	{
		Prob correct = scores[ctr];
		Prob wrong = badScores[ctr];
		Prob term = wrong - correct;
//			cerr<<"\t "<<_args[ctr * 2]<<" term "<<term<<"\n";
		pos.push_back(Feat(ctr, term));
	}

	context.push_back(pos);
	context.push_back(neg);

	_probs.addCount(0, context);

	return log(0);
}

void MixtureModel::initCaches(Document& doc)
{
	JointModel::initCaches(doc);
}

void MixtureModel::clearCaches()
{
	JointModel::clearCaches();
}

Prob MixtureModel::permProbability(Document& doc, ints& perm, bool train)
{
	_sentScores.clear();
	_sentScores.resize(perm.size());
	_globalScore = 0;

	Prob res = 0.0;

	int ctr = 0;
	for(CoherenceModels::iterator i = _models.begin();
		i != _models.end();
		i++, ctr++)
	{
		if((*i)->history() == GLOBAL_HIST)
		{
//careful! models that return GLOBAL_HIST get to see the whole document
//in order, rendering them oblivious to permutations
// 			ints iperm;
// 			randPermutation(iperm, doc.size());
			ints iperm;
			for(int ii = 0; ii < doc.size(); ++ii)
			{
				iperm.push_back(ii);
			}
			Prob term = (*i)->permProbability(doc, iperm, train);
			term *= _probs.weights()[ctr];
			res += term;
			_globalScore += term;
			continue;
		}

		Prob term = (*i)->permProbability(doc, perm, train);
		if(!isfinite(term))
		{
			cerr<<_args[ctr * 2]<<" "<<_args[ctr * 2 + 1]<<" error, prob "<<
				term<<"\n";
		}
		assertLogProb(term);
		term *= _probs.weights()[ctr];
		//cerr<<"Term "<<term<<"\n";
		res += term;
	}

	int size = perm.size();
	ctr = 0;
	for(CoherenceModels::iterator model = _models.begin();
		model != _models.end();
		model++, ctr++)
	{
		for(int ii = 0; ii < size; ++ii)
		{
			_sentScores[ii] += _probs.weights()[ctr] *
				(*model)->sentScores()[ii];
		}
	}

	return res;
}

void MixtureModel::estimate()
{
	_probs.setDimension(_models.size());
	_probs.estimate();
}

void MixtureModel::add(string flag, string file)
{
	addModel(CoherenceModel::loadFromFile(const_cast<char*>(flag.c_str()),
										  const_cast<char*>(file.c_str())));
	_args.push_back(flag);
	_args.push_back(file);
}
