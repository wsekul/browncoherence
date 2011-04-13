==================================
Brown Coherence Toolkit v1.0
==================================

.. contents::

What is this software?
------------------------------

This software contains tools for replicating the main results of our
ACL '11 papers, "Disentangling Chat with Local Coherence Models" and
"Extending the Entity Grid with Entity-Specific Features", and the ACL
'08 paper "Coreference-inspired Coherence Modeling". Hopefully, it is
also a useful general resource for people interested in local
coherence modeling, chat disentanglement, or both. It should be
useable as a plug-and-play replacement for many generative coherence
models, such as entity grids, or, with a little work, a feature
extractor for discriminative models.

The software is copyrighted by its authors, and distributed to you
under the `GPL <http://www.gnu.org/licenses/gpl.html>`_, version 3 or
any subsequent version.

Nearly all the software was written by Micha Elsner, but the tree
package was written by Eugene Charniak and Mark Johnson, and the OWLQN
optimizer is by Jianfeng Gao and Mark Johnson. Support requests
should go to Micha Elsner.

Although there are probably multiple places to get this software
(including the ACL anthology), the most current version is available
from `<https://bitbucket.org/melsner/browncoherence>`_.

Installing the software
------------------------

First off, please *check Bitbucket* to make sure you have the latest
version!

You will need to fill in some file paths. Edit the file
*include/common.h* : change the definition of *DATA_PATH* so that it
points to the *data* directory. (No trailing slash!)

Edit the Makefile to point to your local copy of the GSL and
Wordnet. If you don't have Wordnet, you can turn it off in the
Makefile (the only thing we use is the stemmer, so it's not
particularly important). If you don't have the GSL, you'll need to
recode some things by hand-- this package mostly uses GSL random
number generation, which is easy to replace.

Check that you can make things:

::

	% make everything

Check that you can run things. We have no automatic unit tests yet, so
see the `Using the Software`_ section below and try to run some of the
examples.

To make any particular program we describe below, just type:

::

    % make *program*

Executables are automatically generated in *bin32* on a 32-bit
architecture, and *bin64* on a 64-bit architecture.

Using the software
--------------------

The "important" output from a program goes to STDOUT; debugging
information goes to STDERR.

Document Formats
================

The software operates on files containing Penn Treebank-style
trees. There is an example of an acceptable tree file in
*data/smokedoc.txt*.

Some of the software expects named-entity type labels on NPs. We
assign these by running OPENNLP (available from
`<http://opennlp.sourceforge.net/projects.html>`_) using the invocation::

    zcat *file* | perl -pe 's/^\(S1/(TOP/' | java -Xmx350m opennlp.tools.lang.english.NameFinder -parse opennlp-tools-1.4.1/models/namefind/*.bin.gz | gzip - > *file.out.gz*

The output trees have NE tags under the NP tags. The permissible NE
tags are *person*, *organization*, *location*, *percent*, *time*,
*date* and *money*. An example with an *organization* tag is::

    (TOP (S (NP (DT The) (organization (NNPS Teachers) (NNP Insurance)) (CC and) (NNP Annuity) (NNP Association-College) (NNP Retirement) (NNPS Equities) (NNP Fund)) (VP (VBD said) ...

Some of the software expects Switchboard-style dialogue format. In
this format, each tree is preceded by a tree that looks like:

::

    ( (CODE (SYM SpeakerA1) (. .) ))

Our software uses this snippet to decide the speaker of each
utterance. There is an example of an acceptable dialogue file in
*data/swbd-short.txt*.

Other software uses an extension of this format designed for
multiparty conversational transcripts. Here, the "code" marker tree
contains a little more information:

::

    ( (CODE (SYM SpeakerD) (DIAL 1) (TIME 178) (MENT (SYM A))) )

*SYM* is still a unique speaker ID; *DIAL* is the number of a
conversational thread, and *TIME* is an integer timestamp, measuring
seconds from the beginning of the transcript. *MENT* contains a list
of *SYM* with the identifier letters of speakers who are mentioned in
the utterance. An example of an acceptable transcript file is
*data/transcript-short.txt*.

Data
=================

Work on newswire text uses the Wall Street Journal, TB3 edition,
annotated for named entity as described above with OPENNLP. Training
consists of sections 2-13, and testing of sections 14-24. We actually
use 3-13 to train the individual models, so we can use 2 to train
combinations. We don't use 0 or 1.

Some of our data files are built with scripts from MUC6 and NANC. The
scripts are somewhat messy, so we've just given you the output files.

*train-linkable* is the list of words with coreferents in
MUC. *unlinkable* is the list of words which appear 5 times or more
without any coreferents. (The numbers next to each word are used by
the scripts that build the files, but not meaningful to the model
itself.) *proStats* contains a table of words which occur without
(automatically resolved) coreferent pronouns in NANC, then a table of
words with pronouns in NANC.

Unfortunately, we can't give out all the datasets we use in our chat
study because of licensing restrictions. If you want the synthetic
Switchboard transcripts we use, or the parsed NPS data, just write to
us.

Switchboard Synthetic Data
******************************

If you have a local copy of Switchboard, you can create synthetic
multiparty conversations like ours for yourself. Take the Switchboard
parse files (without edits) in TB-3 format. First, strip backchannels
and other nounless utterances using *script/contentOnly.py*::

   % mkdir [no-backchannels]
   % python script/contentOnly.py [switchboard] [no-backchannels]

Now use *script/multicombine.py* to entangle the dialogues::

   % mkdir [entangled]
   % python script/multicombine.py [no-backchannels] [entangled]

You can edit *multicombine* to determine whether it creates
different-topic or same-topic entanglements, by setting the variable
*makeDiff*. The topic determinations rely on the file
*data/swbd-topics.txt*, which we extracted from the Switchboard header
info.

Linux IRC data
********************

We've provided our transcriptified versions of the Linux dev section
and all six test annotations in *data/linux*; these should allow you
to replicate our results for this dataset.

If you want to process the files yourself, you can get our Linux data
and software from *cs.brown.edu/~melsner*. We parsed the data with our
local installation of the Charniak parser plus some simple postprocess
scripts to discard parse failures and change the tags of "yes", "lol"
and "haha". Then we put it into transcript format using
*script/transcriptify.py*, which requires the original Elsner and
Charniak chat software package. (Make sure the *analysis* library is
in your python path before running this.)::

    % python script/transcriptify.py IRC/dev/linux-dev-0X.annot linux-dev-parse > linux-dev-0X.trans

Adams 08 IRC data
*********************

We can't give out the Adams '08 data ourselves, but you can get it by
request from Craig Martell at the Naval Postgraduate School. It's in
the same format as Linux, so all the tools work exactly the same-- our
preprocessing for these datasets is the same as for Linux.

Looking at Data
===============

**TestSent** shows documents without syntactic annotation::

	% bin32/TestSent data/smokedoc.txt
	data/wsj_0204.mrg
	data/smokedoc.txt
	this is preliminary information , subject to change , and may contain errors .  (data/smokedoc.txt-0)
	any errors in this report will be corrected when the final report has been completed .  (data/smokedoc.txt-1)
	*REST OF DOCUMENT*

.. _`TestGrid`:

**TestGrid** shows the entity grid for a document, which is useful for seeing how the syntactic analysis works::

	% bin32/TestGrid data/smokedoc.txt
	data/smokedoc.txt

                THIS S - - - - - - - - - - - - - - - - - -
         INFORMATION O - - - - - - - - - - - - - - - - - -
              CHANGE X - - - - - - - - - - - - - - - - - -
		*REST OF THE GRID*

Making Models
=============

Each model type has an associated flag. The flag for the
max-likelihood entity grid is *-n*.

The models for our ACL '11 papers are included in a folder called
*models*. Each of their names begins with the appropriate flag; where
we used multiple training sets, we explain the differences below.

The usual way to train a model is **Train**, which writes the model to
standard out::

	% bin32/Train -n data/smokedoc.txt > smokemodel
	Making egrid model with 2 history items,  max salience 4,  smoothing 1,  generativity 1.
	data/smokedoc.txt ... 
	data/smokedoc.txt-1 ... 
	Estimating parameters.

Entity grid / extended entity grid
************************************

Our baseline entity grid is *-n*. Our extended entity grid is
*-f*. You can train them as shown above. We've also provided
pre-trained models *n-wsj.dump* and *f-wsj.dump* in the *models*
directory, so you can just use them.

To test the effect of mention detection, unfortunately, you have to
comment out a block of code (marked by *-- mention --*) in *Sent.cc*.

Linear entity grid
*******************

The logistic-regression entity grid used in the chat paper is *-m*. We
trained this model with a parallel training procedure described in
Mann et al '09, mostly because we wanted to use the same training
scripts as for the topical entity grid. We think the *Train* program
and a little less data will work fine too. (In fact, for the *-f*
model, you should definitely use *Train* and the real WSJ instead of
NANC. Possibly this is because the named entity detector doesn't work
as well on automatic parses but we don't really know.)

To use the parallel trainer, split your data into folds (we used
10). For each fold, run feature extraction using **Featurize**::

    % bin32/Featurize -m data/smokedoc.txt > feats.dump
	Making max ent egrid model with 6 history items,  max salience 4,  generativity 1.
	data/smokedoc.txt ... 
	data/smokedoc.txt-1 ... 

Now estimate a model for each fold using **TrainFromFeatures**::

    % bin32/TrainFromFeatures -m feats.dump > feats-model.dump
	Making max ent egrid model with 6 history items,  max salience 4,  generativity 1.
	Reading from feats.dump ... Opening trace file feats.dump
	0...
	1000...
	Read 1558 datapoints.
	1558 effective samples, 935 points, 59319 parameters.
	iteration 1
	weight vector norm 0
	gradient norm 2843.37
	LL 2159.85
	*MORE GRADIENT OPTIMIZATION*

(For real data, you would have done each of these steps 10 times.)

Finally, average the results using **Average**::

    % bin32/Average -m feats-model.dump [9 other models] > final-model.dump
	Making max ent egrid model with 6 history items,  max salience 4,  generativity 1.
	Model is: feats-model.dump
	Loading max ent entity grid.

(The scripts we used to do this are specific to our computing cluster,
so we're not distributing them, but they aren't very complicated.)

We trained *models/m-model.dump* on parsed Fisher data, and
*models/m-wsj.dump* on parsed NANC. (The NANC data is files 000
through 003 of the McClosky self-training dataset, and the Fisher data
is all the files with indices less than 1200

Topical entity grid
********************

The topical entity grid from the chat paper is *-v*. To make this
model, you have to run two steps, LDA and then parameter learning.

You can make data for LDA using **DocWordMatrix**. You have to edit
this program to provide it with the correct path to your working
directory (down at the bottom). Then you can run Blei's LDA code
(which you get from his Princeton website)::

    % bin32/DocWordMatrix [files] > matrix
    % lda est 1 200 data/settings.txt seeded [working dir]

Now you have to edit the code in *Featurize*, *TrainFromFeatures* and
*Average* to give the correct path to your LDA working directory, and
then run the training procedure above.

We've provided our LDA output (*data/ldaSwbd* and *data/ldaWSJ*), and
our model outputs *models/v-model.dump* and *models/v-wsj.dump*, which
have the same training sets as before.

IBM-1
*******

The IBM-1 model we use in our combination experiments has the flag
*-ww*. This model uses distributed training. You can use **CreateIBM**
to create an initial (stub) model::

    % bin32/CreateIBM -ww > initialIBM
    Making IBM model with 1 context sentences, 1 topics, emission prior 0.1.

Use **DistributedExpectations** on your entire corpus; [start index]
and [end index] tell the program which documents within the corpus to
process, so you should give each parallel process a different range of
indices::

   % bin32/DistributedExpectations -ww ibminit [start index] [end index] data/smokedoc.txt > foldx.dump
   Making IBM model from file...
   data/smokedoc.txt-1 ... 
   Likelihood: 0

Use **CombineAndMax** to run the M-step on all the dump files from the
previous step::

    % bin32/CombineAndMax -ww ibminit [all fold dump files] > step1.dump
    Making IBM model from file...
    Reading foldx.dump...
    Estimating parameters.

Then replace *ibminit* with *step1.dump* and do it all over again-- at
least 5 times. As before, we use a script, but it won't work on your
system.

Our model is *models/ww-wsj.dump*, which is trained on NANC. Since
it's rather large, we've provided it in bzip2 format-- you need to
unzip it before you use it.

Pronouns
*********

The pronoun code flag is *-wp*. It runs some custom code designed to
work over Charniak's unsupervised pronoun model (which we've given you
in *data/charniakModel.txt*). To adapt the parameters, use
**TransferIBM**::

    % bin32/TransferIBM -wp -ec data/charniakModel.txt data/smokedoc.txt
    Creating base model from data/charniakModel.txt
    Reading Charniak model from file...
    Making pronoun model.

Our model is *models/wp-wsj.dump*.

Discourse-newness
******************

The discourse-new model flag is *-e*. This model can be trained with
*Train* as seen above, or we've provided a pre-trained model as
*models/e-wsj.dump*.

Time
*************

The flag for this model is *-t*.

The histogram of time gaps is estimated by
**script/sampleTimes.py**. This expects a sorted list of time gaps.
You can get this using the **analysis/deltaTHist.py** program from
Elsner and Charniak's chat package, or use the one we provide in
*data/chatDeltaT.py*::

   % python analysis/printDeltaT.py IRC/dev/linux-dev-0X.annot > timeGaps
   % python script/sampleTimes.py timeGaps > timeHist

Finally, you edit the **Train** program to point to the correct
histogram file (the *timeHist* file you just made) and run::

    % bin32/Train -t /dev/null > timeModel

(This just makes some minor format changes to your histogram.)

Our time model for our Switchboard simulated chats is
*models/t-model.dump*; the one estimated from the Linux development
set is *models/t-irc.dump*.

Speaker
*************

The flag for this model is *-k*. We tuned the *alpha* parameter by
hand. You can set it in *Train*, or just hack the model once it's
written to disk. Again, the trainer doesn't really do anything and you
can run it on */dev/null*.

Our model (for all datasets) is *models/k-model.dump*.

Mention
***************

The flag for this model is *-a*. For this one, you need to train on a
transcript file with properly assigned *MENT* nodes in the *CODE*
trees. To train on the individual threads of a transcript as if they
were individual documents, use **TrainOnThreads**::

   % bin32/TrainOnThreads  -a data/linux/linux-dev-0x.trans > mentModel
   Making address by name model.
   data/linux-dev-0x.trans ... 

Our model is *models/a-model.dump*, trained on Linux dev as shown. It
doesn't make sense to use this on Switchboard because our tools don't
bother trying to detect name mentions.

Combinations
****************

Log-linear model combinations have a *-x* flag. You need to train
these with *TrainDiscMixture* and *TrainWSJDiscMixture* to optimize
discrimination performance, or *TrainMixture* to optimize single
utterance disentanglement. In either case, you give all the models
with their flags as arguments, followed by some training
documents. *TrainWSJDiscMixture* works on ordinary documents and
permutes their utterances. *TrainDiscMixture* works on Switchboard
dialogues and permutes their turns. Don't use the documents you
trained the component models on, because you'll overfit. (For
newswire, we train combinations on WSJ section 2)::

   % bin32/TrainWSJDiscMixture -n models/n-model.dump -ww models/ww-model.dump [path-to]/wsj/2/*
   Model is: models/m-model.dump
   Loading max ent entity grid.
   Model is: models/ww-model.dump
   Making IBM model from file...
   *GRADIENT OPTIMIZATION*

We provide several models in the *models* directory. *x-mvwpww-wsj*
and *x-mvwpww-swbd* are mixtures for discrimination. *x-mvwpwwt-diff*
and *x-mvwpwwt-same* are for disentanglement on Switchboard synthetic
instances with different and same topics, respectively. *xtkam-irc*
and *xtka-irc* are for disentanglement on IRC, trained on the Linux
development section. For WSJ experiments, the models are
*x-fwpwwe-wsj* and *x-nwpwwe-wsj*. (The naming convention is *x*
followed by the list of component model flags, so *xtka* is the
baseline and *xtkam* is the baseline plus entity grid.)

Examining a Model
=================

Model files are plain text, though they may not be particularly
intelligible. For instance, the *smokemodel* entity grid we
demonstrated how to create with *Train* looks like this::

	NAIVE
	2       4       1       1
	1
	0
		0       0.25
		1       0.25
		2       0.25
		3       0.25

	>>
	*MORE PARAMETERS*

There is a **Print** program that translates these parameters into
(sometimes) useful output. **Print** isn't always guaranteed to do anything
interesting; exactly what the output looks like depends on the model type::

	% bin32/Print -n smokemodel
	Model is: smokemodel
	Loading entity grid.
	[S S 2]:
		S:      0.25
		O:      0.25
		X:      0.25
		-:      0.25

Testing Models
==============

Discrimination
****************

The binary discrimination task is a simple ordering evaluation. It
tests the model's ability to distinguish between a human-authored
document in its original order, and a random permutation of that
document.

There are several ways to run this test. The simplest is to use
**DiscriminateRand**; this program reads any number of documents and
performs the test on each one, using 20 random permutations. This is
the mode in which we ran ordering tests on *WSJ*. The program prints
the raw number of wins, ties and tests to STDOUT::

   % bin32/DiscriminateRand -f models/f-wsj.dump [path-to-wsj]/test/*
   Model is: models-for-test/f-wsj.dump
   Loading max ent naive entity grid with more features.
   Gold score: -114.55
   Score: -118.335 WIN
   Score: -119.055 WIN
   Score: -119.714 WIN
   Score: -126.861 WIN
   Score: -117.529 WIN
   Score: -135.7 WIN
   Score: -136.25 WIN
   Score: -118.743 WIN
   Score: -128.603 WIN
   Score: -135.034 WIN
   Score: -112.473 LOSE
   Score: -128.03 WIN
   Score: -128.247 WIN
   Score: -136.18 WIN
   Score: -120.852 WIN
   Score: -127.633 WIN
   Score: -128.307 WIN
   Score: -130.382 WIN
   Score: -116.738 WIN
   Score: -120.842 WIN
   *LOTS OF DOCUMENTS*
   20080 tests, 16891 wins, 226 ties.
   Mean score: -750.41.
   Total score: -1.58216e+07.
   Mean gold score: -737.508.
   Total gold score: -740458.
   Average margin: 270.937.
   Accuracy: 0.841185 F: 0.845946.

The accuracy and F-scores are on the last line; notice that the
F-score (which differs from the accuracy due to different handling of
ties) is 84.6-- this is slightly different from the 84.5 reported in
the paper because we parallelize some of the tests, which changes the
random seeding slightly.

We permute turns instead of utterances for Switchboard. We used a
fixed set of permuted documents which we wrote out to files and
compared them with **Discriminate**, which reads a single gold
document, followed by a set of permuted versions::

	% bin/Discriminate -m models/m-model.dump [gold file] [bad files]

However, you can also perform this test on random permutations using
**DiscriminateDiscRand**. It works the same way as *DiscriminateRand*::

    % bin32/DiscriminateDiscRand -m models/m-model.dump [path-to-swbd]/test/*.mrg
	  *LOTS OF DOCUMENTS*
	3080 tests, 2611 wins, 0 ties.
	Mean score: -963.063.
	Total score: -3.11455e+06.
	Mean gold score: -957.876.
	Total gold score: -147513.
	Average margin: 108.921.
	Accuracy: 0.847727 F: 0.847727.

The F-score of 84.8 differs from the paper's 86.0 (corresponding to
2611 wins instead of 2650) because, again, we're using a different set
of random permutations. (All models in the paper are compared on the
same permutations.)

Insertion
*************

The insertion test finds the optimal place to insert each sentence
into the document, given the correct ordering of the other
sentences. It is quadratic in document length. It is typically more
difficult than discrimination. The program reports two scores:
perfect insertions and a positional score. (We report only the perfect
insertions.)

You can run this test using **Insert**. (We parallelize our tests, and
advise you do the same, as insertion can be quite time-consuming.) The
program prints the mean positional score, and the number of sentences
correctly inserted, to STDOUT::

    % bin32/Insert -n models/n-wsj.dump data/smokedoc.txt
    Model is: models/n-wsj.dump
    Loading entity grid.
    data/smokedoc.txt ...
	this is preliminary information , subject to change , and may contain errors .  (data/smokedoc.txt-0)
	*REST OF ORIGINAL DOC*
    this is preliminary information , subject to change , and may contain errors .  (data/smokedoc.txt-0)
    	0:	-504.181:	1
		1:	-504.433:	0.888889
		2:	-500.375:	0.777778
    	3:	-499.704:	0.666667
    	4:	-500.857:	0.555556
		5:	-500.857:	0.444444
		6:	-499.301:	0.333333
		7:	-499.625:	0.222222
		8:	-500.476:	0.111111
		9:	-500.149:	0
		10:	-500.314:	-0.111111
		11:	-500.314:	-0.222222
		12:	-499.301:	-0.333333
		13:	-499.301:	-0.444444
		14:	-499.301:	-0.555556
		15:	-499.301:	-0.666667
		16:	-499.771:	-0.777778
		17:	-499.543:	-0.888889
		18:	-499.138:	-1
	Removed: 0	Inserted: 18
	Score: -1
    *REST OF SENTENCES*
	Document mean: -0.77008
	Document perfect: 0 of 19

	Mean: -0.77008
	Perfect: 0 of 19
	Perfect (by line): 0
	Perfect (by doc): 0
	Mean (by line): -0.77008

Disentangling a single utterance
*************************************

You can disentangle a single utterance using **RankDisentangle**,
which plainly makes sense only for a transcript::

    % bin32/RankDisentangle -x models/xtka-irc data/linux/test0.clean.trans
 	Model is: models/xtka-irc
	Loading mixture model.
	Model is: models/k-model.dump
	Loading speaker model.
	Model is: models/a-model.dump
	Loading address by name model.
	Model is: models/t-irc.dump
	Loading time gap model.
	Reading the gold file data/linux/test0.clean.trans ...
	True objective -3270.16
	Transcript length: 791
	Processing:
	what should i choose between slackware 11.0 and redhat 5 ? i 'm looking for a great desktop envoiernment , etc  (A D0: 15381 data/linux/test0.clean.trans-0)
	WIN
	*MANY MORE SENTENCES*
	0 ties.
	Correctly decided 0.990895 tests (54848 of 55352)
	Completely correct on 0.778761 tests (616 of 791)
	Rank 0.990895

The "correctly decided" and "rank" statistics give the relatively
uninteresting number of times the system rejects putting an utterance
in an incorrect thread (that is, if utterance 10 can be part of all 80
threads in the transcript, there are 79 "decisions" to make). The
interesting statistic is how many utterances are "completely correct"
(assigned to the correct thread ahead of all other threads). For this
annotation of the Linux test set, the baseline gets 77.9% with 616
sentences correct. The paper's figure of 74% is averaged over all 6
independent annotations (test0 through test5).

Disentangling a transcript
***************************

You can disentangle a whole transcript using **TestDisentangle**::

    % bin32/TestDisentangle -m models/m-model.dump data/transcript-short.txt
	Model is: models/m-model.dump
	Loading max ent entity grid.
	Reading the gold file data/transcript-short.txt ...
	Selecting binary disentanglement...
	True objective -246.151
	Current objective -266.131
	Moving 6 for gain 1.56512
	Current objective -268.505
	Moving 1 for gain 0.655975
	*LOTS OF SEARCH*
	Best objective -244.118
	T2 1 S1 :  hello . 
	T2 5 S3 :  okay , mary . 
	T2 20 S2 :  yes . 
	T2 31 S0 :  hello , 
	T2 32 S1 :  hi . 
	T2 44 S0 :  hello 
	*REST OF DOCUMENT*

This program searches for a while, then prints out the document in the
same format as our original chat tools, which you can use to score the
output. To get an chat-style printout of the gold data, use
**ChatStyleTranscript**::

   % bin32/ChatStyleTranscript data/transcript-short.txt 
   Reading the gold file data/transcript-short.txt ...
   T1 1 S1 :  hello . 
   T2 5 S3 :  okay , mary . 
   T2 20 S2 :  yes . 
   T1 31 S0 :  hello , 

Programming with the software
------------------------------------

The code uses a fair amount of inheritance, and it's often worthwhile
looking at the ancestor classes before trying to figure out how a
derived class works. The base class for all models is
**CoherenceModel**. The typical life of a CoherenceModel:

* Create a new model with the first constructor.
* Call *train* on all the training documents. *train* should be called
  only before *estimate*.
* Call *estimate* to estimate the parameters (normalize things, run
  EM, perform gradient descent ... ). *estimate* should be called only
  once.
* You may now call *write* to write out a machine-readable version of
  the model. *write* often only writes out the estimated model
  parameters, so you should call it after *estimate*.
* The istream-based constructor calls *read*. This is the inverse of
  *write*.
* Call *logProbability* on all the testing documents.

If you want to evaluate the model on many different permutations of a
document, you will use *permProbability*. This is the main method for
both training and testing the model; *train* and *logProbability* both
call it internally. Models are allowed to cache information about a
document that doesn't vary with order (for instance, the number of
occurrences of a word). Therefore, you must call *initCaches* before
invoking *permProbability*. When you are done using *permProbability*,
call *clearCaches*.
