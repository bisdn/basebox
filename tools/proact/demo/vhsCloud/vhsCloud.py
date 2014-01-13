#!/usr/bin/python

import sys
import time
sys.path.append('../../..')
import logging
import proact.vhscore.vhscore

CONFFILE='vhsCloud.conf'

if __name__ == "__main__":
    vhsCore = proact.vhscore.vhscore.VhsCore(conffile=CONFFILE)
    vhsCore.run()

