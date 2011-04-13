class Inserter;

#ifndef INSERTER_H
#define INSERTER_H

#include "common.h"
#include "CoherenceModel.h"

class Inserter
{
public:
	Inserter(CoherenceModel* model, Document& doc, bool verbose);

	virtual ~Inserter();

	//returns preferred insertion pos for sentence k
	virtual int insert(int k);

	virtual double score(int pos, int k);
	//inserted at pos, doc length n, real position k (0-indexed)
	virtual double metric(int pos, int n, int k);
	//expected distance of pos from k (0-indexed) in doc of length n
	virtual double norm(int n, int k);

protected:
	CoherenceModel* _model;
	Document* _doc;
	bool _verbose;
};

#endif
