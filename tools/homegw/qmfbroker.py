#!/usr/bin/python

import homegw
import qmfconsole



class QmfBrokerInvalError(BaseException):
    def __init__(self, serror=""):
        self.serror = serror
    def __str__(self):
        return "<QmfBrokerInvalError: " + str(self.serror) + " >"



class QmfBroker(object):
    """
    
    """
    def __init__(self, xdpid, brokerUrl = "amqp://localhost:5672"):
        """
        xdpid is currently ignored, but is supposed to carry a QMF independent 
        identifier for an xdpd instance  
        """
        self.qmfConsole = qmfconsole.QmfConsole(brokerUrl)
        self.xdpid = xdpid


    def getVhsGatewayInstance(self):
        vhsgws = self.qmfConsole.getObjects(_class="gateway", _package="de.bisdn.vhsgw")
        # TODO: check for xdpid and return the right xdpd instance
        return vhsgws[0] 


    def vhsAttach(self, type, cpeTunnelID, cpeSessionID, cpeIP, cpeUdpPort):
        result = self.getVhsGatewayInstance().vhsAttach(type, cpeTunnelID, cpeSessionID, cpeIP, cpeUdpPort)
        return (result['vhsTunnelID'], result['vhsSessionID'], result['vhsIP'], result['vhsUdpPort'])


    def vhsDetach(self, cpeTunnelID, cpeSessionID):
        self.getVhsGatewayInstance().vhsDetach(cpeTunnelID, cpeSessionID)
        

    def getDptCoreInstance(self):
        dptcores = self.qmfConsole.getObjects(_class="dptcore", _package="de.bisdn.dptcore")
        # TODO: check for xdpid and return the right xdpd instance
        return dptcores[0] 
        
        
    def vethLinkCreate(self, devname1, devname2):
        self.getDptCoreInstance().vethLinkCreate(devname1, devname2)
        
        
    def vethLinkDestroy(self, devname):
        self.getDptCoreInstance().vethLinkDestroy(devname)
        
        
    def linkAddIP(self, devname, ipaddr, prefixlen, defroute):
        self.getDptCoreInstance().linkAddIP(devname, ipaddr, prefixlen, defroute)
        
        
    def linkDelIP(self, devname, ipaddr, prefixlen):
        self.getDptCoreInstance().linkDelIP(devname, ipaddr, prefixlen)
        
        
    def l2tpCreateTunnel(self, tunnel_id, peer_tunnel_id, remote, local, udp_sport, udp_dport):
        self.getDptCoreInstance().l2tpCreateTunnel(tunnel_id, peer_tunnel_id, remote, local, udp_sport, udp_dport)


    def l2tpDestroyTunnel(self, tunnel_id):
        self.getDptCoreInstance().l2tpDestroyTunnel(tunnel_id)
    
    
    def l2tpCreateSession(self, name, tunnel_id, session_id, peer_session_id):
        self.getDptCoreInstance().l2tpCreateSession(name, tunnel_id, session_id, peer_session_id)
        
        
    def l2tpDestroySession(self, tunnel_id, session_id):
        self.getDptCoreInstance().l2tpDestroySession(tunnel_id, session_id)
        
        
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
            raise QmfBrokerInvalError("param dpid1 is missing")
        if not 'dpid2' in kwargs:
            raise QmfBrokerInvalError("param dpid2 is missing") 
        result = self.getXdpdInstance(self.xdpid).lsiCreateVirtualLink(kwargs['dpid1'], kwargs['dpid2'])
        return [ result.outArgs['devname1'], result.outArgs['devname2'] ]
    
    
    