#!/usr/bin/python

import homegw
import qmfconsole

class DataPath(object):
    """
    
    """
    def __init__(self, xdpid, brokerUrl = "amqp://localhost:5672"):
        """
        xdpid is currently ignored, but is supposed to carry a QMF independent 
        identifier for an xdpd instance  
        """
        self.qmfConsole = qmfconsole.QmfConsole(brokerUrl)
        self.xdpid = xdpid
        
        
    def getXdpdInstance(self, xdpid):
        xdpds = self.qmfConsole.getObjects(_class="xdpd", _package="de.bisdn.xdpd")
        # TODO: check for xdpid and return the right xdpd instance
        return xdpds[0] 
           
           
    def getLsiInstance(self, xdpid, dpid):
        lsis = self.qmfConsole.getObjects(_class="lsi", _package="de.bisdn.xdpd")
        for lsi in lsis:
            if lsi.dpid != dpid:
                continue
            return lsi
        

    def lsiCreate(self, dpid, dpname, ofversion, ntables, ctlaf, ctladdr, ctlport, reconnect):
        """
        create a new LSI instance in the xdpd instance
        """
        self.getXdpdInstance(self.xdpid).lsiCreate(dpid, dpname, ofversion, ntables, ctlaf, ctladdr, ctlport, reconnect)
        
           
    def lsiDestroy(self, dpid):
        """
        destroy an LSI instance
        """
        self.getXdpdInstance(self.xdpid).lsiDestroy(dpid)
        
        
    def portAttach(self, dpid, devname):
        """
        attach a port to an dpid
        """
        print "portAttach " + devname
        self.getLsiInstance(self.xdpid, dpid).portAttach(dpid, devname)


    def portDetach(self, dpid, devname):
        """
        detach a port to an dpid
        """
        print "portDetach " + devname
        self.getLsiInstance(self.xdpid, dpid).portDetach(dpid, devname)


        