#!/usr/bin/python

import homegw
import qmfconsole



class DataPathInvalError(BaseException):
    def __init__(self, serror=""):
        self.serror = serror
    def __str__(self):
        return "<DataPathInvalError: " + str(self.serror) + " >"



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


    def lsiCreateVirtualLink(self, **kwargs):
        """
        create a virtual link between two LSIs
        kwargs['dpid1']
        kwargs['dpid2']
        returns list with devname1 and devname2: ['ge0', 'ge1']
        """
        print "lsiCreateVirtualLink "
        if not 'dpid1' in kwargs:
            raise DataPathInvalError("param dpid1 is missing")
        if not 'dpid2' in kwargs:
            raise DataPathInvalError("param dpid2 is missing") 
        result = self.getXdpdInstance(self.xdpid).lsiCreateVirtualLink()
        return [ result.outArgs['devname1'], result.outArgs['devname2']