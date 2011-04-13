from __future__ import division
import sys
from random import randrange

def sample_multinomial(probs):
    """Gives a random sample from the unnormalized multinomial distribution
    probs, returned as the index of the sampled element."""
    norm = sum(probs)
    if norm == 0:
        return randint(0, len(probs) - 1)

    sample = random()
    total = 0.0
    for x, prob in enumerate(probs):
        total += prob / norm
        if sample < total:
            return x
    raise ValueError("Failed to sample from %s, sample was %s, norm was %s" % \
        (probs, sample, norm))

def sampleHist(hist, buckets):
    bucket = sample_multinomial(hist)
    if bucket != 0:
        bmin = buckets[bucket - 1]
    else:
        bmin = -1
    bmax = buckets[bucket]
    return randrange(bmin + 1, bmax + 1)

def addToHist(item, hist, buckets):
    for ind,bucket in enumerate(buckets):
        if item <= bucket:
            hist[ind] += 1
            break

def getTimeHist(fname):
    #print "Times from", fname
    line = file(fname).read()
    dts = eval(line)
    #print dts

    buckets = []
    seenItem = 0
    currItem = 0
    biggestIncr = 0
    currIncr = 0
    for it in dts:
        seenItem += 1
        if it != currItem:
            if seenItem > 4 and currIncr >= biggestIncr:
                seenItem = 0
                buckets.append(currItem)
                currIncr = 0
            currItem = it
            currIncr += 1
            biggestIncr = max(currIncr, biggestIncr)
    buckets.append(currItem)
    #print buckets

    counts = [0] * len(buckets)
    for item in dts:
        addToHist(item, counts, buckets)
    return dts, counts, buckets

if __name__ == "__main__":
    if len(sys.argv) >= 2:
        fname = sys.argv[1]
    else:
        fname = "data/chatDeltaT.txt"
    print "Data from", fname

    (dts, counts, buckets) = getTimeHist(fname)

    counts2 = [0] * len(buckets)
    for ii in range(1000000):
        item = sampleHist(counts, buckets)
        addToHist(item, counts2, buckets)

    norm = sum(counts)
    counts = [x/norm for x in counts]

#    print buckets

    for ct,bucket in zip(counts, buckets):
        print "\t", bucket, ct
    print ">>"
    
    norm = sum(counts2)
    counts2 = [x/norm for x in counts2]

#     for c1,c2 in zip(counts, counts2):
#         print "%.3g" % c1, "%.3g" % c2
