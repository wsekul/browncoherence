from path import path
import sys
import random
from combineDialogues import merge

if __name__ == "__main__":
    topFile = path("data/swbd-topics.txt")

    SOURCEPATH = path(sys.argv[1])
    topics = {}
    for line in file(topFile):
        (doc,top) = line.split()
        topics[path(doc).stripext()] = top

    topics2 = dict([(x,y) for (x,y) in topics.items()
                    if x.startswith("sw") and
                    (SOURCEPATH/("%s.mrg" % x)).exists()])

    outdir = path(sys.argv[2])
    created = []

    #read created transcripts so as not to recreate them;
    #useful if you need to create extra documents to add to a set
    already = path("/path/to/previously-created")
    if already.exists():
        for ff in already.files():
            (first, second) = ff.basename().split("-")
            print first, second
            if first > second:
                (first, second) = (second, first)
            if (first, second) in created:
                print "Error", first, second
            created.append((first, second))

    #set to true to make different topic transcripts, false for same topic
    makeDiff = True

    for dn in range(100):
        if makeDiff:
            while True:
                first = random.choice(topics2.keys())
                t1 = topics2[first]
                second = random.choice(topics2.keys())
                t2 = topics2[second]
                if first > second:
                    (first, second) = (second, first)

                if t1 != t2 and (first, second) not in created:
                    break
        else:
            while True:
                first = random.choice(topics2.keys())
                t1 = topics2[first]
                others = [(x,y) for (x,y) in topics2.items()
                          if y == t1 and x != first]
                if others:
                    second,t2 = random.choice(others)
                    if first > second:
                        (first, second) = (second, first)

                    if (first, second) not in created:
                        break

        print first, second
        created.append((first, second))
        outf = outdir/("%s-%s" % (first, second))
        merge([SOURCEPATH/("%s.mrg" % first),
               SOURCEPATH/("%s.mrg" % second)], file(outf, 'w'))
