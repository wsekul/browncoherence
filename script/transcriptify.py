from analysis.chatStats import *
import sys

def getCode(name, speakerCodes):
    code = ""
    codeNum = len(speakerCodes)
    if codeNum == 0:
        code = 'A'
    while codeNum > 0:
        cpt = chr(ord('A') + (codeNum % 26))
        code = cpt + code
        codeNum /= 26
    speakerCodes[name] = code

if __name__ == "__main__":
    ch = readChat(sys.argv[1])
    par = file(sys.argv[2])

    speakerCodes = {}
    threadOrdinals = {}

    for sent in ch:
        if sent.thread == -1:
            continue
        if not any([ch.isalpha() for ch in sent.rest]):
            continue

        parSt = par.next().strip()
        if sent.name not in speakerCodes:
            getCode(sent.name, speakerCodes)
        spkCode = speakerCodes[sent.name]

        if sent.thread not in threadOrdinals:
            newThread = len(threadOrdinals)
            threadOrdinals[sent.thread] = newThread
        thread = threadOrdinals[sent.thread]

        ments = []
        for mentioned in sent.mentioned:
            if mentioned not in speakerCodes:
                getCode(mentioned, speakerCodes)
            ments.append(speakerCodes[mentioned])

        if ments:
            mentBlock = " (MENT %s) " % \
                        " ".join(["(SYM %s)" % ment for ment in ments])
        else:
            mentBlock = ""

        codeSnippet = (
            "( (CODE (SYM Speaker%s) (DIAL %d) (TIME %d)%s) )" %
            (spkCode, thread, sent.time, mentBlock))
        print codeSnippet
        print parSt
