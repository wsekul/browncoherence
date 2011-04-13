#include "MaxEntSelection.h"

#include "OWLQN.h"

MaxEntSelection::MaxEntSelection(int n, gsl_rng* random):
	_dim(n),
	_prior(1.0),
	_random(random),
	_samples(0),
	_weights(new Prob[_dim]),
	_spool(NULL)
{
	for(int i=0; i<_dim; ++i)
	{
		_weights[i] = 0;
	}
}

MaxEntSelection::MaxEntSelection(int n, double alpha, gsl_rng* random):
	_dim(n),
	_prior(alpha),
	_random(random),
	_samples(0),
	_weights(new Prob[_dim]),
	_spool(NULL)
{
	for(int i=0; i<_dim; ++i)
	{
		_weights[i] = 0;
	}
}

MaxEntSelection::MaxEntSelection(MaxEntSelection& other):
	_dim(other._dim),
	_prior(other._prior),
	_random(other._random),
	_samples(0),
	_weights(new Prob[_dim]),
	_spool(other._spool)
{
	for(int i=0; i<_dim; ++i)
	{
		_weights[i] = other._weights[i];
	}
}

MaxEntSelection::~MaxEntSelection()
{
	delete[] _weights;

	if(_spool)
	{
		delete _spool;
	}
}

int MaxEntSelection::dimension()
{
	return _dim;
}

void MaxEntSelection::setDimension(int newD)
{
	if(_dim != newD)
	{
		Prob* newWeights = new Prob[newD];

		for(int i=0; i < newD && i < _dim; ++i)
		{
			newWeights[i] = _weights[i];
		}
		for(int i=_dim; i < newD; ++i)
		{
			newWeights[i] = 0;
		}

		_dim = newD;
		delete[] _weights;
		_weights = newWeights;
	}
}

double MaxEntSelection::samples()
{
	return _samples;
}

Prob* MaxEntSelection::weights()
{
	return _weights;
}

double& MaxEntSelection::prior()
{
	return _prior;
}

void MaxEntSelection::read(istream& is)
{
	is>>_prior;
	is>>_dim;
	delete[] _weights;
	_weights = new Prob[_dim];
	for(int i = 0; i < _dim; i++)
	{
		is>>_weights[i];
		//XXX; this made some previous versions of the system work better
		//on IRC single-sent disentanglement.
		//I think it was mostly bogus.
//		_weights[i] *= 10;
	}

	is>>_samples;
}

void MaxEntSelection::write(ostream& os)
{
	os<<_prior<<"\t";
	os<<_dim<<"\n";
	for(int i = 0; i < _dim; i++)
	{
		os<<_weights[i]<<" ";
	}
	os<<"\n";

	os<<_samples<<"\n";
}

void MaxEntSelection::writeFeatures(bool on)
{
	if(on)
	{
		_spool = new boost::iostreams::filtering_ostream();
		_spool->push(boost::iostreams::bzip2_compressor());
		//_spool->push(boost::iostreams::file_sink(_filename));
		_spool->push(cout);
		assert(!_spool->fail());
	}
	else
	{
		if(_spool)
		{
			delete _spool;
		}
		_spool = NULL;
	}
}

Prob MaxEntSelection::readFeatures(string& filename)
{
	writeFeatures(false);

	boost::iostreams::filtering_istream ifs;
    ifs.push(boost::iostreams::bzip2_decompressor());
    ifs.push(boost::iostreams::file_source(filename));

	cerr<<"Opening trace file "<<filename.c_str()<<"\n";
	assert(!ifs.fail());

	return readFeatures(ifs);
}

Prob MaxEntSelection::readFeatures(boost::iostreams::filtering_istream& ifs)
{
	int items = 0;

	int produced;
	while(ifs>>produced)
	{
		if(produced == -3)
		{
			break;
		}

		Prob count;
		ifs>>count;

		MaxEntContext context;
		bool done = false;

		int feat = 0;

		while(true)
		{
			if(done)
			{
				break;
			}

			Feats feats;

			while(true)
			{
				ifs>>feat;
				if(feat == -1)
				{
					context.push_back(feats);
					break;
				}
				else if(feat == -2)
				{
					done = true;
					break;
				}

				Prob fval;
				ifs>>fval;
				feats.push_back(Feat(feat, fval));
			}
		}

// 		cerr<<"\n";
// 		cerr<<produced<<" ";
// 		foreach(MaxEntContext, fv, context)
// 		{
// 			foreach(Feats, feat, *fv)
// 			{
// 				cerr<<feat->first<<":"<<feat->second<<" ";
// 			}
// 		}
// 		cerr<<"\n";

		if(items % 1000 == 0)
		{
			cerr<<items<<"...\n";
		}

		++items;
		addCount(produced, context, count);
	}

	cerr<<"Read "<<items<<" datapoints.\n";

	return 0;
}

void MaxEntSelection::printContext(MaxEntContext& context, ostream& out)
{
	foreach(MaxEntContext, item, context)
	{
		out<<"[ ";
		foreach(Feats, feat, *item)
		{
			out<<feat->first<<":"<<feat->second<<" ";
		}
		out<<"]";
	}
}

void MaxEntSelection::printMarginals(MaxEntContext& context, ostream& out)
{
	int ct = 0;
	foreach(MaxEntContext, item, context)
	{
		out<<"[ ";
		foreach(Feats, feat, *item)
		{
			out<<feat->first<<":"<<feat->second<<"->"<<
				_weights[feat->first]<<" ";
		}
		out<<"] = ";
//		out<<probToken(ct, context);
		out<<weight(context[ct]);
		out<<"\n";
		ct++;
	}
}

void MaxEntSelection::addCount(int selected, MaxEntContext& context,
							   double count)
{
	_samples += count;

	assert(selected < context.size());

	if(!_spool)
	{
		Example* addTo = NULL;
		if(!_examples.empty())
		{
			Example* prevItem = _examples.back();
			if(*prevItem->context == context)
			{
				addTo = prevItem;
			}
		}
		if(!addTo)
		{
			//needed to avoid evoking the copy constructor and deleting
			_examples.push_back(new Example(context));
			addTo = _examples.back();
		}

		addTo->counts[selected] += count;
	}
	else
	{
		(*_spool)<<selected<<" "<<count<<" ";
		for(MaxEntContext::iterator choice = context.begin();
			choice != context.end();
			++choice)
		{
			foreach(Feats, feat, *choice)
			{
				if(_spool)
				{
					(*_spool)<<feat->first<<" "<<feat->second<<" ";
				}
			}
			if(_spool)
			{
				(*_spool)<<" -1 ";
			}
		}
		if(_spool)
		{
			(*_spool)<<" -2 \n";
		}
	}
}

MaxEntContext* MaxEntSelection::mostRecentContextPtr()
{
	assert(!_examples.empty());
	Example* prevItem = _examples.back();	
	return prevItem->context;
}

Prob MaxEntSelection::probToken(int selected, 
								MaxEntContext& context)
{
	assert(selected < context.size());

	double term = weight(context[selected]);
	double denom = norm(context);

//	cerr<<"\nTERM "<<term<<" NORM "<<denom<<"\n";
	assert(denom > 0);

	return term / denom;
}

int MaxEntSelection::sample(MaxEntContext& context)
{
	assert(_random != NULL);

	doubleToInt probs;
	double tot = 0;
	int csize = context.size();
	for(int sel = 0; sel < csize; ++sel)
	{
		double term = weight(context[sel]);
		if(term > 0)
		{
			tot += term;
			probs[tot] = sel;
		}
	}

	double prob = gsl_ran_flat(_random, 0., tot);
	return probs.lower_bound(prob)->second;
}

void MaxEntSelection::estimate(bool verbose, Prob tol, Prob l1)
{
#ifdef SELECTION_MEMOIZE
	_cache.clear();
#endif

	{
// 		if(_examples.size() == 0)
// 		{
// 			return;
// 		}

		MaxEntLikelihood fn(_prior, _examples);

		OWLQN owlqn;

		if(verbose)
		{
			cerr<<_samples<<" effective samples, "<<
				_examples.size()<<" points, "<<_dim<<" parameters.\n";
		}

		Probs init(_dim);
		for(int ii = 0; ii < _dim; ++ii)
		{
			init[ii] = _weights[ii];
		}
		Probs res(_dim);

		if(tol != 0)
		{
			owlqn.Minimize(fn, init, res, l1, tol, 10);
		}
		else
		{
			owlqn.Minimize(fn, init, res, l1, 1e-7, 10);
		}

		if(verbose)
		{
			cerr<<fn.iterations<<" owlqn iters\n";
		}

		for(int ii = 0; ii < _dim; ++ii)
		{
			_weights[ii] = res[ii];
		}
	}
}

void MaxEntSelection::clear()
{
	foreach(Examples, ex, _examples)
	{
		delete *ex;
	}
	_examples.clear();
	_samples = 0;
}

double MaxEntSelection::norm(MaxEntContext& context, int n, Prob weights[])
{
	double denom = 0;
	int csize = context.size();

//	cerr<<csize<<" elements in ctx\n";
	for(int i = 0; i < csize; ++i)
	{
		denom += MaxEntSelection::weight(context[i], n, weights);

//		cerr<<"weight "<<MaxEntSelection::weight(context[i], n, weights)<<
//			" den "<<denom<<"\n";
	}

	return denom;
}

double MaxEntSelection::norm(MaxEntContext& context)
{
	return MaxEntSelection::norm(context, _dim, _weights);
}

double MaxEntSelection::weight(Feats& feats, int n, Prob weights[])
{
	double res = 0;

	foreach(Feats, feat, feats)
	{
		if(feat->first >= n)
		{
			cerr<<"WARN "<<feat->first<<" "<<n<<"\n";
		}
		assert(feat->first < n);

		res += feat->second * weights[feat->first];
	}

	Prob val = exp(-res);
	if(!isfinite(val))
	{
		//cerr<<"Warning: weight overflows!\n";
		return 1e10; // a generic big number
	}
	if(val == 0)
	{
		//cerr<<"Warning: weight underflows!\n";
		//return 1e-10; // a generic small number
	}

	return val;
}

double MaxEntSelection::weight(Feats& feats)
{
#ifdef SELECTION_MEMOIZE
	FeatsProbMap::iterator entry = _cache.find(&feats);
	if(entry != _cache.end())
	{
		return entry->second;
	}
#endif
	Prob res = MaxEntSelection::weight(feats, _dim, _weights);
#ifdef SELECTION_MEMOIZE
	_cache[&feats] = res;
#endif
	return res;
}

MaxEntLikelihood::MaxEntLikelihood(double prior, Examples& its):
	iterations(0),
	_prior(prior),
	_examples(&its),
	verbose(1)
{
}

double MaxEntLikelihood::operator() (int n, double x[], double grad[])
{
	iterations++;

	for(int i = 0; i < n; i++)
	{
		grad[i] = 0;
	}

	double ll = 0;
	double samples = 0;

	FeatsProbMap shortcut;

	if(verbose)
	{
		cerr<<"iteration "<<iterations<<"\n";
	}

	foreach(Examples, itemPtr, *_examples)
	{
//		cerr<<"Example:\n";
		Example* item = *itemPtr;

		MaxEntContext& context = *item->context;
		doubles& counts = item->counts;
		double totChoices = 0;
		foreach(doubles, ct, counts)
		{
			totChoices += *ct;
		}
		samples += totChoices;

		Prob norm = 0;
		foreach(MaxEntContext, feats, context)
		{
			Prob weight;
			if(shortcut.find(&*feats) == shortcut.end())
			{
				weight = MaxEntSelection::weight(*feats, n, x);
				shortcut[&*feats] = weight;
			}
			else
			{
				weight = shortcut[&*feats];
			}
			norm += weight;
		}

//		cerr<<"Calculated norm: "<<norm<<"\n";

		int ct = 0;
		for(MaxEntContext::iterator choice = context.begin();
			choice != context.end();
			++choice, ++ct)
		{
			Prob curr;
			if(norm > 0)
			{
				curr = shortcut[&*choice] / norm;
			}
			else
			{
				curr = 1.0/context.size();
			}
			if(!isProb(curr))
			{
				cerr<<" curr "<<curr<<" norm "<<norm<<"\n";
			}
			assertProb(curr);

			//check for this case?
// 			if(curr < 1e-20)
// 			{
// 				curr = 1e-20;
// 			}

			double chosen = counts[ct];

			ll += chosen * log(curr);

			foreach(Feats, feat, *choice)
			{
//				cerr<<"\tf"<<feat->first<<" "<<feat->second<<"\n";
				grad[feat->first] += chosen * (1 - curr) * feat->second +
					(totChoices - chosen) * (0 - curr) * feat->second;
			}

//			cerr<<(ct == chosen)<<": calculated prob "<<curr<<"\n";
		}
	}

//	cerr<<"\n\n";

	//regularization
	for(int i = 0; i < n; i++)
	{
		//l2 regularization
		ll -= _prior * x[i] * x[i];

//		cerr<<i<<"\t"<<x[i]<<" prior "<<_prior<<" grad "<<grad[i]<<"\n";
		grad[i] += 2 * _prior * x[i];
		assert(isfinite(grad[i]));
//		cerr<<"grad now "<<grad[i]<<"\n";

//		cerr<<i<<"\tx "<<x[i]<<"\tdx "<<grad[i]<<"\n";
	}

	ll = -ll;
	assert(isfinite(ll));

	if(verbose)
	{
		Prob xn = 0;
		for(int i = 0; i < n; i++)
		{
			xn += x[i] * x[i];
		}
		cerr<<"weight vector norm "<<sqrt(xn)<<"\n";
		Prob gn = 0;
		for(int i = 0; i < n; i++)
		{
			gn += grad[i] * grad[i];
		}
		cerr<<"gradient norm "<<sqrt(gn)<<"\n";
		cerr<<"LL "<<ll<<"\n\n";
	}

	return ll;
}

double MaxEntLikelihood::Eval(const DblVec& x, DblVec& grad)
{
	iterations++;

	int n = x.size();

	for(int i = 0; i < n; i++)
	{
		grad[i] = 0;
	}

	double ll = 0;
	double samples = 0;

	FeatsProbMap shortcut;

	if(verbose)
	{
		cerr<<"iteration "<<iterations<<"\n";
	}

	foreach(Examples, itemPtr, *_examples)
	{
//		cerr<<"Example:\n";
		Example* item = *itemPtr;

		MaxEntContext& context = *item->context;
		doubles& counts = item->counts;
		double totChoices = 0;
		foreach(doubles, ct, counts)
		{
			totChoices += *ct;
		}
		samples += totChoices;

		Prob norm = 0;
		foreach(MaxEntContext, feats, context)
		{
			Prob weight;
			if(shortcut.find(&*feats) == shortcut.end())
			{
				//take addr of x[0], and assume the vector is contiguous in
				//memory so this is the array ptr
				weight = MaxEntSelection::weight(*feats, n, 
												 const_cast<double*>(&x[0]));
				shortcut[&*feats] = weight;
			}
			else
			{
				weight = shortcut[&*feats];
			}
//			cerr<<"Weight "<<weight<<"\n";
			norm += weight;
		}

//		cerr<<"Calculated norm: "<<norm<<"\n";

		int ct = 0;
		for(MaxEntContext::iterator choice = context.begin();
			choice != context.end();
			++choice, ++ct)
		{
			Prob curr;
			if(norm > 0)
			{
				curr = shortcut[&*choice] / norm;
			}
			else
			{
				curr = 1.0/context.size();
			}
			if(!isProb(curr))
			{
				cerr<<" curr "<<curr<<" norm "<<norm<<"\n";
			}
			assertProb(curr);

			//check for this case?
			if(curr < 1e-30)
			{
				curr = 1e-30;
			}

			double chosen = counts[ct];

			ll += chosen * log(curr);
			assertLogProb(ll);

			foreach(Feats, feat, *choice)
			{
//				cerr<<"\tf"<<feat->first<<" "<<feat->second<<"\n";
				grad[feat->first] += chosen * (1 - curr) * feat->second +
					(totChoices - chosen) * (0 - curr) * feat->second;
			}

//			cerr<<ct<<" "<<chosen<<": calculated prob "<<curr<<"\n";
		}
	}

//	cerr<<"\n\n";

	//regularization
	for(int i = 0; i < n; i++)
	{
		//l2 regularization
		ll -= _prior * x[i] * x[i];

//		cerr<<i<<"\t"<<x[i]<<" prior "<<_prior<<" grad "<<grad[i]<<"\n";
		grad[i] += 2 * _prior * x[i];
		assert(isfinite(grad[i]));
//		cerr<<"grad now "<<grad[i]<<"\n";

//		cerr<<i<<"\tx "<<x[i]<<"\tdx "<<grad[i]<<"\n";
	}

	ll = -ll;
	assert(isfinite(ll));

	if(verbose)
	{
		Prob xn = 0;
		for(int i = 0; i < n; i++)
		{
			xn += x[i] * x[i];
		}
		cerr<<"weight vector norm "<<sqrt(xn)<<"\n";
		Prob gn = 0;
		for(int i = 0; i < n; i++)
		{
			gn += grad[i] * grad[i];
		}
		cerr<<"gradient norm "<<sqrt(gn)<<"\n";
		cerr<<"LL "<<ll<<"\n";
	}

	return ll;
}
