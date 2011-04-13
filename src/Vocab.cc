#include "Vocab.h"

Vocab::Vocab():
	_invClean(true)
{
}

Vocab::Vocab(Vocab& other):
	_vocab(other._vocab),
	_directVocab(other._directVocab),
	_invVocab(other._invVocab),
	_invClean(other._invClean)
{
}

void Vocab::clear()
{
	_vocab.clear();
	_directVocab.clear();
	_invVocab.clear();
	_invClean = true;
}

void Vocab::write(ostream& os)
{
	writeHashMap(_vocab, os);
}

void Vocab::read(istream& is)
{
	clear();
	readHashMap(_vocab, is);
	_invClean = false;
}

int Vocab::size()
{
	return _vocab.size();
}

int Vocab::get(int label, bool add)
{
	symIntMap::iterator dv = _directVocab.find(label);
	if(dv != _directVocab.end())
	{
		return dv->second;
	}
	else
	{
		int vInd = get(SYMTAB->toString(label), add);
		_directVocab[label] = vInd;
		return vInd;
	}
}

int Vocab::get(const string& word, bool add)
{
	assert(word != "");
	strIntMap::iterator sv = _vocab.find(word);
	if(sv != _vocab.end())
	{
		return sv->second;
	}
	else
	{
		if(add)
		{
			int newKey = _vocab.size();
			_vocab[word] = newKey;
			_invClean = false;
			return newKey;
		}
		else
		{
			return -1;
		}
	}
}

string& Vocab::inv(int vi)
{
	if(!_invClean)
	{
		invert();
	}
	return _invVocab[vi];
}

void Vocab::invert()
{
	if(!_invClean)
	{
		//invert the vocab table
		for(strIntMap::iterator voc = _vocab.begin();
			voc != _vocab.end();
			voc++)
		{
			_invVocab[voc->second] = voc->first;
		}
		_invClean = true;
	}
}

void Vocab::translate(Vocab& other, intIntMap& res)
{
	foreach(strIntMap, word, other._vocab)
	{
		int key = get(word->first, true);
		res[word->second] = key;
	}
}

/////////////////////////////////////////

IntVocab::IntVocab():
	_invClean(true)
{
}

IntVocab::IntVocab(IntVocab& other):
	_directVocab(other._directVocab),
	_invVocab(other._invVocab),
	_invClean(other._invClean)
{
}

void IntVocab::clear()
{
	_directVocab.clear();
	_invVocab.clear();
	_invClean = true;
}

void IntVocab::write(ostream& os)
{
	writeHashMap(_directVocab, os);
}

void IntVocab::read(istream& is)
{
	clear();
	readHashMap(_directVocab, is);
	_invClean = false;
}

int IntVocab::size()
{
	return _directVocab.size();
}

int IntVocab::get(unsigned int label, bool add)
{
	symIntMap::iterator dv = _directVocab.find(label);
	if(dv != _directVocab.end())
	{
		return dv->second;
	}
	else
	{
		if(add)
		{
			unsigned int newKey = _directVocab.size();
			_directVocab[label] = newKey;
			_invClean = false;
			return newKey;
		}
		else
		{
			return -1;
		}
	}
}

unsigned int IntVocab::inv(int vi)
{
	if(!_invClean)
	{
		invert();
	}
	return _invVocab[vi];
}

void IntVocab::invert()
{
	if(!_invClean)
	{
		//invert the vocab table
		for(symIntMap::iterator voc = _directVocab.begin();
			voc != _directVocab.end();
			voc++)
		{
			_invVocab[voc->second] = voc->first;
		}
		_invClean = true;
	}
}

void IntVocab::translate(IntVocab& other, intIntMap& res)
{
	foreach(symIntMap, word, other._directVocab)
	{
		int key = get(word->first, true);
		res[word->second] = key;
	}
}
