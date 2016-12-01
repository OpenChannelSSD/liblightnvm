#!/usr/bin/env python
from subprocess import Popen, PIPE

NSECTORS = 4
NPLANES = 4

def main():

    ppas = []

    for i in xrange(0, 16):
        ch = 0
        lun = 0
        blk = 3
        pg = 0
        sec = i % NSECTORS
        pl = (i / NSECTORS) % NPLANES

        cmd = [str(x) for x in [
            "nvm_addr",
            "fmt_g",
            "/dev/nvme0n1",
            ch, lun, pl, blk, pg, sec
        ]]

        process = Popen(cmd, stdout=PIPE, stderr=PIPE)
        out, err = process.communicate()
        print(out.strip())
        ppas.append(out[1:17])

    print("ppas{ %s }" % " ".join(ppas))

if __name__ == "__main__":
    main()
