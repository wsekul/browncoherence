from __future__ import division
import sys
from sampleTimes import getTimeHist, sampleHist
from random import randint, randrange, normalvariate

def merge(swbds, out):
    dialogues = []

    for ff in swbds:
        lines = file(ff).readlines()
        times = []
        speakers = []
        newLines = []
        currSpkr = None
        currT = randint(0, 5)
        for line in lines:
            if "CODE" not in line:
                dt = int(abs(round(normalvariate(0, 2.5))))
                times.append(currT)
                speakers.append(currSpkr)
                newLines.append(line.strip())
                currT += dt
            else:
                if "SpeakerA" in line:
                    currSpkr = "A"
                else:
                    currSpkr = "B"

        dialogues.append(zip(newLines, times, speakers))

    #xxx truncate to length of shortest
    minLen = min([len(x) for x in dialogues])
    for ct in range(len(dialogues)):
        dialogues[ct] = dialogues[ct][:minLen]

    while any([len(x) for x in dialogues]):
        possNext = []

        soonest = None
        for ind,di in enumerate(dialogues):
            if di and (di[0][1] < soonest or soonest is None):
                soonest = di[0][1]
                possNext = [ind]
            elif di and di[0][1] == soonest:
                possNext.append(ind)

        #print soonest, possNext

        assert(soonest is not None)
        next = randrange(len(possNext))
        dial = dialogues[possNext[next]]
        nextLine = dial.pop(0)

        dial = possNext[next]

        codeAtom = "( (CODE (SYM Speaker%s) (DIAL %d) (TIME %d)) )" %\
                   (chr(ord(nextLine[2]) + 2 * dial), dial, nextLine[1])
        print >>out, codeAtom
        print >>out, nextLine[0]


if __name__ == "__main__":
    swbds = sys.argv[1:]
    merge(swbds, sys.stdout)
