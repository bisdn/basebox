#!/usr/bin/python

import sys
import time
sys.path.append('/root/git/vmcore/tools')
import logging
import proact.common.xdpdproxy
import proact.vhscore.vhscore

CONFFILE='vhsCloud.conf'
QMFBROKER='172.16.250.76'
XDPDID='vhs-xdpd-0'


class Lsi(object):
    def __init__(self, dpname, dpid, ofversion=3, ntables=1, ctlaf=2, ctladdr='127.0.0.1', ctlport=6633, reconnect=2):
        self.dpname = dpname
	self.dpid = dpid
	self.ofversion = ofversion
	self.ntables = ntables
	self.ctlaf = ctlaf
	self.ctladdr = ctladdr
	self.ctlport = ctlport
	self.reconnect = reconnect
	self.ports = []

if __name__ == "__main__":

    logger = logging.getLogger('proact')
    logger.setLevel(logging.DEBUG)
    fh = logging.FileHandler('vhsCloud.log')
    fh.setLevel(logging.DEBUG)
    ch = logging.StreamHandler()
    ch.setLevel(logging.ERROR)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    fh.setFormatter(formatter)
    ch.setFormatter(formatter)
    logger.addHandler(fh)
    logger.addHandler(ch)

    lsiList = {}
    
    lsiList['router'] = Lsi('router', 1000, 3, 4, 2, QMFBROKER, 6633)
    lsiList['router'].ports.append('ge1')
    lsiList['router'].ports.append('vethR00')
    lsiList['router'].ports.append('vethR10')
    
    lsiList['etherswitch'] = Lsi('etherswitch', 2000, 3, 4, 2, QMFBROKER, 6644)
    lsiList['etherswitch'].ports.append('ge0')
    lsiList['etherswitch'].ports.append('vethS00')
    lsiList['etherswitch'].ports.append('vethS10')

    xdpdProxy = proact.common.xdpdproxy.XdpdProxy(QMFBROKER, XDPDID)
    print xdpdProxy
    
    createLink = False
    for dpname, lsi in lsiList.iteritems():
        try:
            xdpdProxy.getLsi(lsi.dpid)
        except proact.common.xdpdproxy.XdpdProxyException:
            createLink = True
            xdpdProxy.createLsi(lsi.dpname, lsi.dpid, lsi.ofversion, lsi.ntables, lsi.ctlaf, lsi.ctladdr, lsi.ctlport, lsi.reconnect)
            for devname in lsi.ports:
            	xdpdProxy.attachPort(lsi.dpid, devname)

    if createLink:
        (devname1, devname2) = xdpdProxy.createVirtualLink(lsiList['router'].dpid, lsiList['etherswitch'].dpid)

        vhsCore = proact.vhscore.vhscore.VhsCore(conffile=CONFFILE, lanLinks=[devname1])
        vhsCore.run()

	for dpname, lsi in lsiList.iteritems():
		print 'delete LSI ' + str(lsi.dpid)
		xdpdProxy.destroyLsi(lsi.dpid)			
