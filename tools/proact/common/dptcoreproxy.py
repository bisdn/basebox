#!/usr/bin/python

import sys
sys.path.append('../..')

import proact.common.qmfhelper
import time
import qmf2


class DptCoreProxyException(Exception):
    def __init__(self, serror):
        self.serror = serror
    def __str__(self):
        return '<DptCoreProxyException: ' + str(self.serror) + ' >'


class DptCoreProxy(proact.common.qmfhelper.QmfConsole):    
    def __init__(self, brokerUrl = '127.0.0.1', dptCoreID=''):
        proact.common.qmfhelper.QmfConsole.__init__(self, brokerUrl)
        
        #for k, v in kwargs.iteritems():
        #    setattr(self, k, v)
        
        # identifier of xdpd instance to be used
        #
        self.dptCoreID = dptCoreID
        
        
    def __str__(self):
        #sstr = ''
        #for attribute in dir(self):
        #    sstr += attribute + ':' + str(getattr(self, attribute)) + ' ' 
        return '<DptCoreProxy dptCoreID: ' + str(self.dptCoreID) + '>'


    def cleanUp(self):
        pass


    def __getDptCoreHandle(self):
        dptCoreHandles = self.getObjects(_class='dptcore', _package='de.bisdn.proact.dptcore')
        for dptCoreHandle in dptCoreHandles:
            print dptCoreHandle
            if dptCoreHandle.dptCoreID == self.dptCoreID:
                return dptCoreHandle
        else:
            raise DptCoreProxyException('DptCore instance for dptCoreID: ' + self.dptCoreID + ' not found on QMF bus')


    def addVethLink(self, devname1, devname2):
        try:
            self.__getDptCoreHandle().vethLinkCreate(devname1, devname2)
        except:
            raise DptCoreProxyException('unable to create veth pair (' + devname1 + ',' + devname2 + ')')


    def delVethLink(self, devname):
        try:
            self.__getDptCoreHandle().vethLinkDestroy(devname)
        except:
            raise DptCoreProxyException('unable to destroy veth pair with devname (' + devname + ')')

    def addAddr(self, devname, ipaddr, prefixlen, defroute):
        try:
            self.__getDptCoreHandle().linkAddIP(devname, ipaddr, prefixlen, defroute)
        except:
            raise DptCoreProxyException('unable to add address ' + str(ipaddr) + '/' + str(prefixlen) + ' on devname (' + devname + ')')

    def delAddr(self, devname, ipaddr, prefixlen):
        try:
            self.__getDptCoreHandle().linkDelIP(devname, ipaddr, prefixlen)
        except:
            raise DptCoreProxyException('unable to delete address ' + str(ipaddr) + '/' + str(prefixlen) + ' on devname (' + devname + ')')

    def addL2tpTunnel(self, tunnel_id, peer_tunnel_id, remote, local, udp_sport, udp_dport):
        try:
            self.__getDptCoreHandle().l2tpCreateTunnel(tunnel_id, peer_tunnel_id, remote, local, udp_sport, udp_dport)
        except:
            raise DptCoreProxyException('unable to add L2TP tunnel with tunnel_id ' + str(tunnel_id) + ' and peer_tunnel_id ' + str(peer_tunnel_id))
    
    def delL2tpTunnel(self, tunnel_id):
        try:
            self.__getDptCoreHandle().l2tpDestroyTunnel(tunnel_id)
        except:
            raise DptCoreProxyException('unable to delete L2TP tunnel with tunnel_id ' + str(tunnel_id))
    
    def addL2tpSession(self, name, tunnel_id, session_id, peer_session_id):
        try:
            self.__getDptCoreHandle().l2tpCreateSession(name, tunnel_id, session_id, peer_session_id)
        except: 
            raise DptCoreProxyException('unable to add L2TP session (' + str(name) + ') with tunnel_id ' + str(tunnel_id) 
                                        + ', session_id ' + str(session_id) + ' and peer_session_id ' + str(peer_session_id))
    
    def delL2tpSession(self, tunnel_id, session_id):
        try:
            self.__getDptCoreHandle().l2tpDestroySession(tunnel_id, session_id)
        except:
            raise DptCoreProxyException('unable to delete L2TP session with tunnel_id ' + str(tunnel_id) + ' and session_id ' + str(session_id))
    

if __name__ == "__main__":
    dptCoreProxy = DptCoreProxy('127.0.0.1', 'vhs-xdpd-0')
    print DptCoreProxy
    
    dptCoreProxy.createLsi('dp0', 1000) 
    dptCoreProxy.createLsi('dp1', 2000)
    time.sleep(4)
    dptCoreProxy.attachPort(1000, 'veth0')
    time.sleep(2)
    dptCoreProxy.attachPort(1000, 'veth2')
    time.sleep(2)
    
    print dptCoreProxy.createVirtualLink(1000, 2000)
    time.sleep(2)
    
    dptCoreProxy.detachPort(1000, 'veth2')
    time.sleep(2)
    dptCoreProxy.detachPort(1000, 'veth0')
    time.sleep(4)
    dptCoreProxy.destroyLsi(2000)
    dptCoreProxy.destroyLsi(1000)
    
    
    
    