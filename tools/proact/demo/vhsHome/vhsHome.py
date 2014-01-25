#!/usr/bin/python

import sys
import time
sys.path.append('../../..')
import logging
import proact.hgwcore.hgwcore

CONFFILE='vhsHome.conf'

if __name__ == "__main__":
    hgwCore = proact.hgwcore.hgwcore.HgwCore(conffile=CONFFILE)
    hgwCore.run()

