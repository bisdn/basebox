#!/usr/bin/python

import qmfhelper
import select
import sys
import os
from pyroute2.netlink.iproute import *
from pyroute2 import IPRoute
from pyroute2 import IPDB



class Route(object):
    """ abstraction for a route in a basecore instance """
    def __init__(self, baseCore, family, dst, dstlen, table, type, scope):
        self.baseCore   = baseCore
        self.family     = family
        self.dst        = dst
        self.dstlen     = dstlen
        self.table      = table
        self.type       = type
        self.scope      = scope

    def __str__(self):
        return '<Route: family:' + str(self.family) + \
            ' dst:' + self.dst + ' dstlen:' + str(self.dstlen) + \
            ' table:' + str(self.table) + ' type:' + str(self.type) + ' scope:' + str(self.scope) + ' >' 

    def activated(self):
        self.baseCore.addEvent(BaseCoreEvent(self, BaseCore.EVENT_NEW_ROUTE))
    
    def deactivated(self):
        self.baseCore.addEvent(BaseCoreEvent(self, BaseCore.EVENT_DEL_ROUTE))


class Address(object):
    """ abstraction for an address in a basecore instance """
    def __init__(self, baseCore, ifindex, family, addr, prefixlen, scope):
        self.baseCore   = baseCore
        self.ifindex    = ifindex
        self.family     = family
        self.addr       = addr
        self.prefixlen  = prefixlen
        self.scope      = scope
    
    def __str__(self):
        return '<Address: ifindex:' + str(self.ifindex) + ' family:' + str(self.family) + \
            ' addr:' + self.addr + ' prefixlen:' + str(self.prefixlen) + ' scope:' + str(self.scope) + ' >' 

    def __eq__(self, addr):
        if isinstance(addr, Address):
            return ((self.ifindex == addr.ifindex) and \
                    (self.family == addr.family) and \
                    (self.addr == addr.addr) and \
                    (self.prefixlen == addr.prefixlen) and \
                    (self.scope == addr.scope))
        raise NotImplementedError
    
    def activated(self):
        self.baseCore.addEvent(BaseCoreEvent(self, BaseCore.EVENT_NEW_ADDR))
    
    def deactivated(self):
        self.baseCore.addEvent(BaseCoreEvent(self, BaseCore.EVENT_DEL_ADDR))
    

class LinkException(Exception):
    def __init__(self, serror):
        self.serror = serror
    def __str__(self):
        return '<LinkException: ' + self.serror + '>' 
    


class Link(object):
    """ abstraction for a link in a basecore instance """
    def __init__(self, baseCore, ifindex, devname):
        self.baseCore = baseCore
        self.ifindex = ifindex
        self.devname = devname
        self.active = False
        self.addrs = []
    
    def __str__(self):
        s_addrs = ''
        for addr in self.addrs:
            s_addrs += str(addr) + ' '
        return '<Link => devname:' + self.devname + ' active:' + str(self.active) + \
            ' ifindex:' + str(self.ifindex) + ' addresses:' + s_addrs + ' >'
            
    def __eq__(self, link):
        if isinstance(link, Link):
            return ((self.devname == link.devname) and (self.ifindex == link.ifindex))
        raise NotImplementedError
    
    def activated(self):
        self.active = True
        print 'link active: ' + str(self)
        self.baseCore.addEvent(BaseCoreEvent(self, BaseCore.EVENT_NEW_LINK))
        
    def deactivated(self):
        self.active = False
        print 'link not active: ' + str(self)
        self.baseCore.addEvent(BaseCoreEvent(self, BaseCore.EVENT_DEL_LINK))
    
    def handleNewAddr(self, addr, ndmsg):
        for address in self.addrs:
            if addr == address:
                break;
        else:
            self.addrs.append(addr)
            addr.activated()
            print 'link new address: ' + str(self) + ' ' + str(addr)
    
    def handleDelAddr(self, addr, ndmsg):
        for address in self.addrs:
            if addr == address:
                self.addrs.remove(addr)
                addr.deactivated()
                print 'link delete address: ' + str(self) + ' ' + str(addr)
                break


class BaseCoreException(Exception):
    def __init__(self, serror):
        self.serror = serror
    def __str__(self):
        return '<BaseCoreException: ' + self.serror + ' >'


class BaseCoreQmfAgentHandler(qmfhelper.QmfAgentHandler):
    def __init__(self, agentSession):
        self.agentSess = agentSession
        qmfhelper.QmfAgentHandler.__init__(self, agentSession)
        self.start()
        
    def setSchema(self):
        raise BaseCoreException("BaseCoreQmfAgentHandler::setSchema() method not implemented")
    
    def method(self):
        raise BaseCoreException("BaseCoreQmfAgentHandler::method() method not implemented")
    

class BaseCoreEvent(object):
    """Base class for all events generated within class HomeGateway"""
    def __init__(self, source, type, opaque=''):
        self.source = source
        self.type = type
        self.opaque = opaque
    
    def __str__(self, *args, **kwargs):
        return '<BaseCoreEvent [type:' + str(self.type) + '] ' + object.__str__(self, *args, **kwargs) + ' >'



class BaseCore(object):
    """
    overwrite the following methods: cleanUp(), handleEvent(), 
    handleNewRoute(), handleDelRoute(), handleNewLink(), handleDelLink(), handleNewAddr(), handleDelAddr()
    """
    EVENT_QUIT = 1
    EVENT_IPROUTE = 2
    EVENT_NEW_LINK = 3
    EVENT_DEL_LINK = 4
    EVENT_NEW_ADDR = 5
    EVENT_DEL_ADDR = 6
    EVENT_NEW_ROUTE = 7
    EVENT_DEL_ROUTE = 8
    def __init__(self, brokerUrl="127.0.0.1", **kwargs):
        try:
            self.qmfConsole = qmfhelper.QmfConsole(brokerUrl)
            self.vendor = kwargs.get('vendor', 'bisdn.de')
            self.product = kwargs.get('product', 'basecore')
            self.qmfAgent = qmfhelper.QmfAgent(brokerUrl, vendor=self.vendor, product=self.product)
        except qmfhelper.QmfConsoleException:
            raise
        except qmfhelper.QmfAgentException:
            raise
        (self.pipein, self.pipeout) = os.pipe()
        self.events = []
        self.ipr = IPRoute() 
        self.ipr.register_callback(iprouteCallback, lambda x: True, [self, ])
        self.ipdb = IPDB(self.ipr)
        self.links = {}
        self.routes = []
        
        
    def cleanUp(self):
        raise BaseCoreException('BaseCore::cleanUp() method not implemented')
                    
    def handleEvent(self, event):
        raise BaseCoreException('BaseCore::handleEvent() method not implemented')
    
        
    def __handleNewLink(self, ndmsg):
        """
        extracts ifname attribute and calls derived function 
        """
        for attr in ndmsg['attrs']:
            if attr[0] == 'IFLA_IFNAME':
                (ifindex, devname) = (ndmsg['ifindex'], attr[1])
                if not devname in self.links:
                    self.links[devname] = Link(self, ifindex, devname)
                    self.links[devname].activated()
                break
    
    def __handleDelLink(self, ndmsg):
        """
        extracts ifname attribute and calls derived function 
        """
        for attr in ndmsg['attrs']:
            if attr[0] == 'IFLA_IFNAME':
                (ifindex, devname) = (ndmsg['ifindex'], attr[1])
                if devname in self.links:
                    self.links[devname].deactivated()
                break
    
    def __handleNewAddr(self, ndmsg):
        """
        extracts address attribute and calls derived function 
        """
        for attr in ndmsg['attrs']:
            if attr[0] == 'IFA_ADDRESS':
                (ifindex, family, addr, prefixlen, scope) = (ndmsg['index'], ndmsg['family'], attr[1], ndmsg['prefixlen'], ndmsg['scope'])
                for link in self.links:
                    if link.ifindex == ifindex:
                        link.handleNewAddr(Address(self, ifindex, family, addr, prefixlen, scope), ndmsg)
                break
        
    def __handleDelAddr(self, ndmsg):
        """
        extracts address attribute and calls derived function 
        """
        for attr in ndmsg['attrs']:
            if attr[0] == 'IFA_ADDRESS':
                (ifindex, family, addr, prefixlen, scope) = (ndmsg['index'], ndmsg['family'], attr[1], ndmsg['prefixlen'], ndmsg['scope'])
                for link in self.links:
                    if link.ifindex == ifindex:
                        link.handleDelAddr(Address(self, ifindex, family, addr, prefixlen, scope), ndmsg)
                break
    
    def __handleNewRoute(self, ndmsg):
        """
        extracts RTA_DST attribute and calls derived function 
        """
        for attr in ndmsg['attrs']:
            if attr[0] == 'RTA_DST':
                (family, dst, dstlen, table, type, scope) = (ndmsg['family'], attr[1], ndmsg['dst_len'], ndmsg['table'], ndmsg['type'], ndmsg['scope'])
                route = Route(family, dst, dstlen, table, type, scope)
                if not route in self.routes:
                    self.routes.append(route)
                    route.activated() 
                    break

    
    def __handleDelRoute(self, ndmsg):
        """
        extracts RTA_DST attribute and calls derived function 
        """
        for attr in ndmsg['attrs']:
            if attr[0] == 'RTA_DST':
                (family, dst, dstlen, table, type, scope) = (ndmsg['family'], attr[1], ndmsg['dst_len'], ndmsg['table'], ndmsg['type'], ndmsg['scope'])
                route = Route(family, dst, dstlen, table, type, scope)
                if route in self.routes:
                    route.deactived() 
                    break

            
    def addEvent(self, event):
        try:
            if not isinstance(event, BaseCoreEvent):
                raise BaseCoreException('event is not of type BaseCoreEvent')
            self.events.append(event)
            os.write(self.pipeout, str(event.type))
        except BaseCoreException, e:
            print 'ignoring event ' + str(e)
    
    def handleNetlinkEvent(self, event):
        ndmsg = event.opaque
        nevent = ndmsg['event']
        if   nevent == 'RTM_NEWROUTE':
            self.__handleNewRoute(ndmsg)
        elif nevent == 'RTM_NEWLINK':
            self.__handleNewLink(ndmsg)
        elif nevent == 'RTM_NEWADDR':
            self.__handleNewAddr(ndmsg)
        elif nevent == 'RTM_DELROUTE':
            self.__handleDelRoute(ndmsg)
        elif nevent == 'RTM_DELLINK':
            self.__handleDelLink(ndmsg)
        elif nevent == 'RTM_DELADDR':
            self.__handleDelAddr(ndmsg)
    
    def run(self):
        self.ep = select.epoll()
        self.ep.register(self.pipein, select.EPOLLET | select.EPOLLIN)
        print "waiting for work to be done..."
        while True:
            try :
                self.ep.poll(timeout=-1, maxevents=1)
                evType = os.read(self.pipein, 256)
                for event in self.events:
                    if event.type == self.EVENT_QUIT:
                        self.cleanUp()
                        return
                    elif event.type == self.EVENT_IPROUTE:
                        self.handleNetlinkEvent(event)
                    else:
                        self.handleEvent(event)
                else:
                    self.events = []
            except KeyboardInterrupt:
                sys.stdout.write("terminating...")
                self.addEvent(BaseCoreEvent(self, self.EVENT_QUIT))


def iprouteCallback(ndmsg, baseCore):
    if not isinstance(baseCore, BaseCore):
        print "invalid type"
        return
    baseCore.addEvent(BaseCoreEvent("", BaseCore.EVENT_IPROUTE, ndmsg))




if __name__ == "__main__":
    BaseCore(brokerUrl="127.0.0.1").run() 
    
    
    