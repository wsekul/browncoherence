class MaxEntSelection;

#ifndef MAX_ENT_SELECTION
#define MAX_ENT_SELECTION

#include "common.h"

#include "/usr/include/gsl/gsl_rng.h"
#include "/usr/include/gsl/gsl_randist.h"

#include "OWLQN.h"

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>

typedef std::pair<int, Prob> Feat;

//typedef intToProb Feats;
typedef std::vector<Feat> Feats;
typedef std::vector<Feats> MaxEntContext;
typedef std::tr1::unordered_map<Feats*, Prob> FeatsProbMap;

struct BinaryContext : public MaxEntContext
{
	BinaryContext()
	{
		push_back(Feats());
		back().clear();
	}
};

struct Example
{
	Example(MaxEntContext& ct):
		context(new MaxEntContext(ct)),
		counts(context->size(), 0.0),
		owner(true)
	{}

	Example(MaxEntContext* ct):
		context(ct),
		counts(context->size(), 0.0),
		owner(false)
	{}

	~Example()
	{
		if(owner)
		{
			delete context;
		}
	}

	void prune()
	{
		//don't mess with it if someone else owns it!
		//in fact, if you're the owner but there are copies, this is also
		//a terrible idea
		//but there's no explicit check
		assert(owner); 

		int size = context->size();
		MaxEntContext* newContext = new MaxEntContext();
		doubles newCounts;
		for(int ii = 0; ii < size; ++ii)
		{
			if(counts[ii] > 0)
			{
				newContext->push_back((*context)[ii]);
				newCounts.push_back(counts[ii]);
			}
		}

		delete context;
		context = newContext;
		counts = newCounts;
		//cerr<<"Saved "<<(size-counts.size())<<" items!\n";
	}

	MaxEntContext* context;
	doubles counts;
	bool owner;
};


typedef std::vector<Example*> Examples;

class MaxEntSelection
{
public:
	MaxEntSelection(int n, gsl_rng* random=NULL);
	MaxEntSelection(int n, double alpha, gsl_rng* random=NULL);
	MaxEntSelection(MaxEntSelection& other);
	
	virtual ~MaxEntSelection();

	virtual int dimension();
	virtual void setDimension(int newD);

	virtual double samples();

	virtual Prob* weights();
	virtual double& prior();

	virtual void read(istream& is);
	virtual void write(ostream& os);

	virtual void writeFeatures(bool on);
	virtual Prob readFeatures(string& fname);
	virtual Prob readFeatures(boost::iostreams::filtering_istream& ifs);

	static void printContext(MaxEntContext&, ostream& out);
	virtual void printMarginals(MaxEntContext&, ostream& out);

 	virtual void addCount(int selected, MaxEntContext& context,
 						  double count=1.0);

	MaxEntContext* mostRecentContextPtr();

	//probability of selecting a particular token
	virtual Prob probToken(int selected, 
						   MaxEntContext& context);
	virtual int sample(MaxEntContext& context);

	virtual void estimate(bool verbose=true, Prob tol=0, Prob l1=0);

	virtual void clear();

	virtual Prob weight(Feats& features);
	virtual Prob norm(MaxEntContext& context);

	static Prob weight(Feats& features, int n, Prob weights[]);
	static Prob norm(MaxEntContext& context, int n, Prob weights[]);
	static Prob logistic(Prob dot);

//protected:

	int _dim;
	double _prior;
	gsl_rng* _random;
	double _samples;
	Prob* _weights;

#ifdef SELECTION_MEMOIZE
	FeatsProbMap _cache;
#endif

	Examples _examples;

	boost::iostreams::filtering_ostream* _spool;
};

struct MaxEntLikelihood : public DifferentiableFunction
{
	MaxEntLikelihood(double prior, Examples& examples);

	//n is dim[x]
	//x is function argument
	//returns f(x)
	//writes gradient df/dx(x)
	double operator() (int n, double x[], double grad[]);
	double Eval(const std::vector<double>& x, std::vector<double>& grad);

	int iterations;
	double _prior;
	Examples* _examples;
	int verbose;
};

#endif
