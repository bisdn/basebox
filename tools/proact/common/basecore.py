#!/usr/bin/python

import qmfhelper
import select
import sys
import os
from pyroute2.netlink.iproute import *
from pyroute2 import IPRoute
from pyroute2 import IPDB


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
        
        
    def cleanUp(self):
        raise BaseCoreException('BaseCore::cleanUp() method not implemented')
                    
    def handleEvent(self, event):
        raise BaseCoreException('BaseCore::handleEvent() method not implemented')

    def handleNewLink(self, ifindex, devname, ndmsg):
        pass
    
    def handleDelLink(self, ifindex, devname, ndmsg):
        pass
    
    def handleNewAddr(self, ifindex, family, addr, prefixlen, scope, ndmsg):
        pass
        
    def handleDelAddr(self, ifindex, family, addr, prefixlen, scope, ndmsg):
        pass
    
    def handleNewRoute(self, family, dst, dstlen, table, type, scope, ndmsg):
        pass
    
    def handleDelRoute(self, family, dst, dstlen, table, type, scope, ndmsg):
        pass
    
        
    def __handleNewLink(self, ndmsg):
        """
        extracts ifname attribute and calls derived function 
        """
        for attr in ndmsg['attrs']:
            if attr[0] == 'IFLA_IFNAME':
                self.handleNewLink(ndmsg['index'], attr[1], ndmsg)
                break
    
    def __handleDelLink(self, ndmsg):
        """
        extracts ifname attribute and calls derived function 
        """
        for attr in ndmsg['attrs']:
            if attr[0] == 'IFLA_IFNAME':
                self.handleDelLink(ndmsg['index'], attr[1], ndmsg)
                break
    
    def __handleNewAddr(self, ndmsg):
        """
        extracts address attribute and calls derived function 
        """
        for attr in ndmsg['attrs']:
            if attr[0] == 'IFA_ADDRESS':
                self.handleNewAddr(ndmsg['index'], ndmsg['family'], attr[1], ndmsg['prefixlen'], ndmsg['scope'], ndmsg)
                break
        
    def __handleDelAddr(self, ndmsg):
        """
        extracts address attribute and calls derived function 
        """
        for attr in ndmsg['attrs']:
            if attr[0] == 'IFA_ADDRESS':
                self.handleDelAddr(ndmsg['index'], ndmsg['family'], attr[1], ndmsg['prefixlen'], ndmsg['scope'], ndmsg)
                break
    
    def __handleNewRoute(self, ndmsg):
        """
        extracts RTA_DST attribute and calls derived function 
        """
        for attr in ndmsg['attrs']:
            if attr[0] == 'RTA_DST':
                self.handleNewRoute(ndmsg['family'], attr[1], ndmsg['dst_len'], ndmsg['table'], ndmsg['type'], ndmsg['scope'], ndmsg)
                break
    
    def __handleDelRoute(self, ndmsg):
        """
        extracts RTA_DST attribute and calls derived function 
        """
        for attr in ndmsg['attrs']:
            if attr[0] == 'RTA_DST':
                self.handleDelRoute(ndmsg['family'], attr[1], ndmsg['dst_len'], ndmsg['table'], ndmsg['type'], ndmsg['scope'], ndmsg)
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
    
    
    