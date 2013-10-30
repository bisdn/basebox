#!/usr/bin/python

import sys
sys.path.append('../..')

import proact.common.qmfhelper
import time
import qmf2


class XdpdProxyException(Exception):
    def __init__(self, serror):
        self.serror = serror
    def __str__(self):
        return '<XdpdProxyException: ' + str(self.serror) + ' >'


class XdpdProxy(proact.common.qmfhelper.QmfConsole):    
    def __init__(self, brokerUrl = '127.0.0.1', xdpdID=''):
        proact.common.qmfhelper.QmfConsole.__init__(self, brokerUrl)
        
        #for k, v in kwargs.iteritems():
        #    setattr(self, k, v)
        
        # identifier of xdpd instance to be used
        #
        self.xdpdID = xdpdID

        # dictionary of LSI instances: key=dpid, value=XdpdLsiProxy instance
        #
        self.xdpdLsiProxies = {}
        
        
    def __str__(self):
        #sstr = ''
        #for attribute in dir(self):
        #    sstr += attribute + ':' + str(getattr(self, attribute)) + ' ' 
        return '<XdpdProxy xdpdID: ' + str(self.xdpdID) + '>'


    def cleanUp(self):
        for dptHandle in self.xdpdLsiProxies:
            self.__detach(dptHandle.dpid)
        self.xdpdLsiProxies = {}


    def __getXdpdHandle(self):
        xdpdHandles = self.getObjects(_class='xdpd', _package='de.bisdn.xdpd')
        for xdpdHandle in xdpdHandles:
            if xdpdHandle.xdpdID == self.xdpdID:
                print xdpdHandle
                return xdpdHandle
        else:
            raise XdpdProxyException('xdpd instance for xdpdID: ' + self.xdpdID + ' not found on QMF bus')


    def __getLsiHandle(self, dpid):
        lsiHandles = self.getObjects(_class='lsi', _package='de.bisdn.xdpd')
        for lsiHandle in lsiHandles:
            if lsiHandle.dpid == dpid:
                return lsiHandle
        else:
            raise XdpdProxyException('LSI instance dpid: ' + str(dpid) + 'for xdpdID: ' + self.xdpdID + ' not found on QMF bus')



    def __attach(self, dpid):
        """
        attach to the specified remote LSI, create the instance, if necessary
        """
        if not dpid in self.xdpdLsiProxies:
            raise XdpdProxyException('cannot attach dpid: ' + str(dpid))
        try:
            dptlsi = self.xdpdLsiProxies[dpid]
            xdpdHandle = self.__getXdpdHandle()
            if dptlsi.state == XdpdLsiProxy.STATE_DETACHED:
                xdpdHandle.lsiCreate(dptlsi.dpid, dptlsi.dpname, dptlsi.ofversion, dptlsi.ntables,
                                     dptlsi.ctlaf, dptlsi.ctladdr, dptlsi.ctlport, dptlsi.reconnect)
                dptlsi.state = XdpdLsiProxy.STATE_ATTACHED
        except Exception, e:
            print e
            raise
            #raise XdpdProxyException('unable to create remote dpid: ' + str(dpid) + ' on remote datapath with xdpdID: ' + self.xdpdID)
            
    
        
    def __detach(self, dpid):
        """
        detach from specified LSI, destroy it on xdpd
        """
        if not dpid in self.xdpdLsiProxies:
            raise XdpdProxyException('cannot detach dpid: ' + str(dpid))
        try:
            dptlsi = self.xdpdLsiProxies[dpid]
            xdpdHandle = self.__getXdpdHandle()
            if dptlsi.state == XdpdLsiProxy.STATE_ATTACHED:
                xdpdHandle.lsiDestroy(dptlsi.dpid)
                dptlsi.state = XdpdLsiProxy.STATE_DETACHED
        except:
            raise XdpdProxyException('unable to destroy remote dpid: ' + str(dpid) + ' on remote datapath with xdpdID: ' + self.xdpdID)


    def createLsi(self, dpname, dpid, ofversion=3, ntables=1, ctlaf=2, ctladdr='127.0.0.1', ctlport=6633, reconnect=2):
        """
        create a new XdpdLsiProxy object and attach to the remote LSI instance
        """
        if not dpid in self.xdpdLsiProxies:
            self.xdpdLsiProxies[dpid] = XdpdLsiProxy(dpname=dpname, dpid=dpid, ofversion=ofversion,
                                          ntables=ntables, ctlaf=ctlaf, ctladdr=ctladdr,
                                          ctlport=ctlport, reconnect=reconnect)
        self.__attach(dpid)


    def destroyLsi(self, dpid):
        """
        destroy a XdpdLsiProxy object and detach from its associated remote LSI instance
        """
        if not dpid in self.xdpdLsiProxies:
            return
        self.__detach(dpid)
        self.xdpdLsiProxies.pop(dpid, None)


    def createVirtualLink(self, dpid1, dpid2):
        if not dpid1 in self.xdpdLsiProxies:
            raise XdpdProxyException('dpid1: ' + str(dpid1) + ' not found')
        if not dpid2 in self.xdpdLsiProxies:
            raise XdpdProxyException('dpid2: ' + str(dpid2) + ' not found')
        try:
            xdpdHandle = self.__getXdpdHandle()
            dptLsi1 = self.xdpdLsiProxies[dpid1]            
            if dptLsi1.state == XdpdLsiProxy.STATE_DETACHED:
                self.__attach(dpid1)
            dptLsi2 = self.xdpdLsiProxies[dpid2]
            if dptLsi2.state == XdpdLsiProxy.STATE_DETACHED:
                self.__attach(dpid2)
            result = xdpdHandle.lsiCreateVirtualLink(dpid1, dpid2)
            return [result.outArgs['devname1'], result.outArgs['devname2']]
        except:
            raise XdpdProxyException('unable to create virtual link')


    def attachPort(self, dpid, devname):
        """
        attach port 'devname' to LSI referred to by 'dpid'
        """
        if not dpid in self.xdpdLsiProxies:
            raise XdpdProxyException('XdpdLsiProxy for dpid: ' + str(dpid) + ' not found')
        if not self.xdpdLsiProxies[dpid].state == DptLsiProxy.STATE_ATTACHED:
            self.__attach(dpid)
        try:
            lsiHandle = self.__getLsiHandle(dpid).portAttach(dpid, devname)
        except:
            raise XdpdProxyException('unable to attach port ' + str(devname) + ' to LSI dpid: ' + str(dpid))


    def detachPort(self, dpid, devname):
        """
        detach port 'devname' from LSI referred to by 'dpid'
        """
        if not dpid in self.xdpdLsiProxies:
            raise XdpdProxyException('XdpdLsiProxy for dpid: ' + str(dpid) + ' not found')
        if not self.xdpdLsiProxies[dpid].state == DptLsiProxy.STATE_ATTACHED:
            self.__attach(dpid)
        try:
            lsiHandle = self.__getLsiHandle(dpid).portDetach(dpid, devname)
        except:
            raise XdpdProxyException('unable to detach port ' + str(devname) + ' from LSI dpid: ' + str(dpid))



class XdpdLsiProxy(object):
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
        return '<XdpdLsiProxy dpname: ' + str(self.dpname) + \
                    ' dpid: '       + str(self.dpid) + \
                    ' state: '      + str(self.state) + \
                    ' dpname: '     + str(self.dpname) + \
                    ' ofversion: '  + str(self.ofversion) + \
                    ' ntables: '    + str(self.ntables) + \
                    ' ctlaf: '      + str(self.ctlaf) + \
                    ' ctladdr: '    + str(self.ctladdr) + \
                    ' ctlport: '    + str(self.ctlport) + \
                    ' reconnect: '  + str(self.reconnect) + \
                    '>'

    
    

if __name__ == "__main__":
    xdpdProxy = XdpdProxy('127.0.0.1', 'vhs-xdpd-0')
    print xdpdProxy
    xdpdProxy.createLsi('dp0', 1000) 
    xdpdProxy.createLsi('dp1', 2000)
    time.sleep(4)
    xdpdProxy.attachPort(1000, 'veth0')
    time.sleep(2)
    xdpdProxy.attachPort(1000, 'veth2')
    time.sleep(2)
    
    print xdpdProxy.createVirtualLink(1000, 2000)
    time.sleep(2)
    
    xdpdProxy.detachPort(1000, 'veth2')
    time.sleep(2)
    xdpdProxy.detachPort(1000, 'veth0')
    time.sleep(4)
    xdpdProxy.destroyLsi(2000)
    xdpdProxy.destroyLsi(1000)
    
    