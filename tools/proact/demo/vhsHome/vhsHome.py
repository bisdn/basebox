#!/usr/bin/python

import sys
import time
sys.path.append('../../..')
import logging
#import proact.common.xdpdproxy
import proact.hgwcore.hgwcore

CONFFILE='vhsHome.conf'
QMFBROKER='172.16.250.65'
CTLADDR='172.16.250.65'
XDPDID='hgw-xdpd-0'



if __name__ == "__main__":
    hgwCore = proact.hgwcore.hgwcore.HgwCore(conffile=CONFFILE)
    hgwCore.run()

