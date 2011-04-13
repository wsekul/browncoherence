from path import path
import sys

if __name__ == "__main__":
    ind,outd = sys.argv[1:]
    for ff in path(ind).files():
        outf = file(path(outd)/ff.basename(), 'w')
        for line in file(ff):
            if "(CODE" in line or "(NN" in line:
                print >>outf, line.strip()
