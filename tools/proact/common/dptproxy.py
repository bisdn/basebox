#!/usr/bin/python

import sys
sys.path.append('../..')
print sys.path

import proact.common.qmfhelper
import time
import qmf2


class DptProxyException(Exception):
    def __init__(self, serror):
        self.serror = serror
    def __str__(self):
        return '<DptProxyException: ' + str(self.serror) + ' >'


class DptProxy(proact.common.qmfhelper.QmfConsole):    
    def __init__(self, brokerUrl = '127.0.0.1', xdpdID=''):
        proact.common.qmfhelper.QmfConsole.__init__(self, brokerUrl)
        
        #for k, v in kwargs.iteritems():
        #    setattr(self, k, v)
        
        # identifier of xdpd instance to be used
        #
        self.xdpdID = xdpdID

        # dictionary of LSI instances: key=dpid, value=DptLsiProxy instance
        #
        self.dptLsiProxies = {}
        
        
    def __str__(self):
        #sstr = ''
        #for attribute in dir(self):
        #    sstr += attribute + ':' + str(getattr(self, attribute)) + ' ' 
        return '<DptProxy xdpdID: ' + str(self.xdpdID) + '>'


    def cleanUp(self):
        for dptHandle in self.dptLsiProxies:
            self.__detach(dptHandle.dpid)
        self.dptLsiProxies = {}


    def __getXdpdHandle(self):
        xdpdHandles = self.getObjects(_class='xdpd', _package='de.bisdn.xdpd')
        print "AAA"
        for xdpdHandle in xdpdHandles:
            print xdpdHandle
            if xdpdHandle.xdpdID == self.xdpdID:
                return xdpdHandle
        else:
            raise DptProxyException('xdpd instance for xdpdID: ' + self.xdpdID + ' not found on QMF bus')


    def __getLsiHandle(self, dpid):
        lsiHandles = self.getObjects(_class='lsi', _package='de.bisdn.xdpd')
        for lsiHandle in lsiHandles:
            if lsiHandle.dpid == dpid:
                return lsiHandle
        else:
            raise DptProxyException('LSI instance dpid: ' + str(dpid) + 'for xdpdID: ' + self.xdpdID + ' not found on QMF bus')



    def __attach(self, dpid):
        """
        attach to the specified remote LSI, create the instance, if necessary
        """
        if not dpid in self.dptLsiProxies:
            raise DptProxyException('cannot attach dpid: ' + str(dpid))
        try:
            dptlsi = self.dptLsiProxies[dpid]
            print "BBB"
            xdpdHandle = self.__getXdpdHandle()
            print "CCC"
            if dptlsi.state == DptLsiProxy.STATE_DETACHED:
                xdpdHandle.lsiCreate(dptlsi.dpid, dptlsi.dpname, dptlsi.ofversion, dptlsi.ntables,
                                     dptlsi.ctlaf, dptlsi.ctladdr, dptlsi.ctlport, dptlsi.reconnect)
                dptlsi.state = DptLsiProxy.STATE_ATTACHED
        except:
            raise DptProxyException('unable to create remote dpid: ' + str(dpid) + ' on remote datapath with xdpdID: ' + self.xdpdID)
            
    
        
    def __detach(self, dpid):
        """
        detach from specified LSI, destroy it on xdpd
        """
        if not dpid in self.dptLsiProxies:
            raise DptProxyException('cannot detach dpid: ' + str(dpid))
        try:
            dptlsi = self.dptLsiProxies[dpid]
            xdpdHandle = self.__getXdpdHandle()
            if dptlsi.state == DptLsiProxy.STATE_ATTACHED:
                xdpdHandle.lsiDestroy(dptlsi.dpid)
                dptlsi.state = DptLsiProxy.STATE_DETACHED
        except:
            raise DptProxyException('unable to destroy remote dpid: ' + str(dpid) + ' on remote datapath with xdpdID: ' + self.xdpdID)


    def createLsi(self, dpname, dpid, ofversion=3, ntables=1, ctlaf=2, ctladdr='127.0.0.1', ctlport=6633, reconnect=2):
        """
        create a new DptLsiProxy object and attach to the remote LSI instance
        """
        if not dpid in self.dptLsiProxies:
            self.dptLsiProxies[dpid] = DptLsiProxy(dpname=dpname, dpid=dpid, ofversion=ofversion,
                                          ntables=ntables, ctlaf=ctlaf, ctladdr=ctladdr,
                                          ctlport=ctlport, reconnect=reconnect)
        self.__attach(dpid)


    def destroyLsi(self, dpid):
        """
        destroy a DptLsiProxy object and detach from its associated remote LSI instance
        """
        if not dpid in self.dptLsiProxies:
            return
        self.__detach(dpid)
        self.dptLsiProxies.pop(dpid, None)


    def createVirtualLink(self, dpid1, dpid2):
        if not dpid1 in self.dptLsiProxies:
            raise DptProxyException('dpid1: ' + str(dpid1) + ' not found')
        if not dpid2 in self.dptLsiProxies:
            raise DptProxyException('dpid2: ' + str(dpid2) + ' not found')
        try:
            xdpdHandle = self.__getXdpdHandle()
            dptLsi1 = self.dptLsiProxies[dpid1]            
            if dptLsi1.state == DptLsiProxy.STATE_DETACHED:
                self.__attach(dpid1)
            dptLsi2 = self.dptLsiProxies[dpid2]
            if dptLsi2.state == DptLsiProxy.STATE_DETACHED:
                self.__attach(dpid2)
            result = xdpdHandle.lsiCreateVirtualLink(dpid1, dpid2)
            return [result.outArgs['devname1'], result.outArgs['devname2']]
        except:
            raise DptProxyException('unable to create virtual link')


    def attachPort(self, dpid, devname):
        """
        attach port 'devname' to LSI referred to by 'dpid'
        """
        if not dpid in self.dptLsiProxies:
            raise DptProxyException('DptLsiProxy for dpid: ' + str(dpid) + ' not found')
        if not self.dptLsiProxies[dpid].state == DptLsiProxy.STATE_ATTACHED:
            self.__attach(dpid)
        try:
            lsiHandle = self.__getLsiHandle(dpid).portAttach(dpid, devname)
        except:
            raise DptProxyException('unable to attach port ' + str(devname) + ' to LSI dpid: ' + str(dpid))


    def detachPort(self, dpid, devname):
        """
        detach port 'devname' from LSI referred to by 'dpid'
        """
        if not dpid in self.dptLsiProxies:
            raise DptProxyException('DptLsiProxy for dpid: ' + str(dpid) + ' not found')
        if not self.dptLsiProxies[dpid].state == DptLsiProxy.STATE_ATTACHED:
            self.__attach(dpid)
        try:
            lsiHandle = self.__getLsiHandle(dpid).portDetach(dpid, devname)
        except:
            raise DptProxyException('unable to detach port ' + str(devname) + ' from LSI dpid: ' + str(dpid))



class DptLsiProxy(object):
    # data path represented by this proxy is not created on remote side
    STATE_DETACHED = 1
    # data path represented by this proxy has been successfully created on remote side
    STATE_ATTACHED = 2
    
    def __init__(self, **kwargs):
                
        # get all parameters for creating a data path element remotely
        #
        self.dpid       = kwargs.get('dpid', 1000)
        self.dpname     = kwargs.get('dpname', 'vhs-xdpd-dpt-1000')
        self.ofversion  = kwargs.get('ofversion', 3)
        self.ntables    = kwargs.get('ntables', 1)
        self.ctlaf      = kwargs.get('ctlaf', 2)
        self.ctladdr    = kwargs.get('ctladdr', '127.0.0.1')
        self.ctlport    = kwargs.get('ctlport', 6633)
        self.reconnect  = kwargs.get('reconnect', 2)
        
        # internal state
        #
        self.state      = self.STATE_DETACHED
    
    def __str__(self):
        #sstr = ''
        #for attribute in dir(self):
        #    sstr += attribute + ':' + str(getattr(self, attribute)) + ' ' 
        return '<DptLsiProxy dpname: ' + str(self.dpname) + ' dpid: ' + str(self.dpid) + ' state: ' + str(self.state) + '>'

    
    

if __name__ == "__main__":
    dptProxy = DptProxy('127.0.0.1', 'vhs-xdpd-0')
    print dptProxy
    dptProxy.createLsi('dp0', 1000) 
    dptProxy.createLsi('dp1', 2000)
    time.sleep(4)
    dptProxy.attachPort(1000, 'veth0')
    time.sleep(2)
    dptProxy.attachPort(1000, 'veth2')
    time.sleep(2)
    
    print dptProxy.createVirtualLink(1000, 2000)
    time.sleep(2)
    
    dptProxy.detachPort(1000, 'veth2')
    time.sleep(2)
    dptProxy.detachPort(1000, 'veth0')
    time.sleep(4)
    dptProxy.destroyLsi(2000)
    dptProxy.destroyLsi(1000)
    
    