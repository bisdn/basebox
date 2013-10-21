#!/usr/bin/python

from pyroute2.netlink.iproute import *
from pyroute2 import IPRoute
from pyroute2 import IPDB
import dns
from qmf.console import Session
import subprocess
import netaddr
import select
import sys
import dns.resolver
import os
import re
import time
from qmf2 import *
import cqmf2
import cqpid
from l2tp import *
from threading import Thread


brokerUrl = '127.0.0.1'

EVENT_QUIT = 0

class VhsGatewayQmfAgentHandler(AgentHandler):
    """
    QMF agent handler for class VhsGateway
    """
    def __init__(self, vhsgw, agentSession):
        AgentHandler.__init__(self, agentSession)
        self.agentSession = agentSession
        self.package = 'de.bisdn.vhsgw'
        
        
        self.qmfSchemaVhsGateway = Schema(SCHEMA_TYPE_DATA, self.package, "gateway")
        #self.qmfSchemaVhsGateway.addProperty(SchemaProperty("dptcoreid", SCHEMA_DATA_INT))
        
        self.qmfSchemaVhsGateway_testMethod = SchemaMethod("test", desc='testing method for QMF agent support in python')
        self.qmfSchemaVhsGateway_testMethod.addArgument(SchemaProperty("subcmd", SCHEMA_DATA_STRING, direction=DIR_IN))
        self.qmfSchemaVhsGateway_testMethod.addArgument(SchemaProperty("outstring", SCHEMA_DATA_STRING, direction=DIR_OUT))
        self.qmfSchemaVhsGateway.addMethod(self.qmfSchemaVhsGateway_testMethod)

        self.qmfSchemaVhsGateway_vhsAttachMethod = SchemaMethod("vhsAttach", desc='attach to a VHS instance')
        self.qmfSchemaVhsGateway_vhsAttachMethod.addArgument(SchemaProperty("type", SCHEMA_DATA_STRING, direction=DIR_IN_OUT))
        self.qmfSchemaVhsGateway_vhsAttachMethod.addArgument(SchemaProperty("cpeTunnelID", SCHEMA_DATA_INT, direction=DIR_IN))
        self.qmfSchemaVhsGateway_vhsAttachMethod.addArgument(SchemaProperty("cpeSessionID", SCHEMA_DATA_INT, direction=DIR_IN))
        self.qmfSchemaVhsGateway_vhsAttachMethod.addArgument(SchemaProperty("cpeIP", SCHEMA_DATA_STRING, direction=DIR_IN))
        self.qmfSchemaVhsGateway_vhsAttachMethod.addArgument(SchemaProperty("cpeUdpPort", SCHEMA_DATA_INT, direction=DIR_IN))
        self.qmfSchemaVhsGateway_vhsAttachMethod.addArgument(SchemaProperty("vhsIP", SCHEMA_DATA_STRING, direction=DIR_OUT))
        self.qmfSchemaVhsGateway_vhsAttachMethod.addArgument(SchemaProperty("vhsUdpPort", SCHEMA_DATA_INT, direction=DIR_OUT))
        self.qmfSchemaVhsGateway_vhsAttachMethod.addArgument(SchemaProperty("vhsTunnelID", SCHEMA_DATA_INT, direction=DIR_OUT))
        self.qmfSchemaVhsGateway_vhsAttachMethod.addArgument(SchemaProperty("vhsSessionID", SCHEMA_DATA_INT, direction=DIR_OUT))
        self.qmfSchemaVhsGateway.addMethod(self.qmfSchemaVhsGateway_vhsAttachMethod)

        self.qmfSchemaVhsGateway_vhsDetachMethod = SchemaMethod("vhsDetach", desc='detach from a VHS instance')
        self.qmfSchemaVhsGateway_vhsDetachMethod.addArgument(SchemaProperty("type", SCHEMA_DATA_STRING, direction=DIR_IN))
        self.qmfSchemaVhsGateway_vhsDetachMethod.addArgument(SchemaProperty("cpeTunnelID", SCHEMA_DATA_INT, direction=DIR_IN))
        self.qmfSchemaVhsGateway_vhsDetachMethod.addArgument(SchemaProperty("cpeSessionID", SCHEMA_DATA_INT, direction=DIR_IN))
        self.qmfSchemaVhsGateway.addMethod(self.qmfSchemaVhsGateway_vhsDetachMethod)

        # add further methods here ...

        self.agentSession.registerSchema(self.qmfSchemaVhsGateway)

        self.vhsgw = vhsgw
        self.qmfDataVhsGateway = Data(self.qmfSchemaVhsGateway)
        self.qmfAddrVhsGateway = self.agentSession.addData(self.qmfDataVhsGateway, 'singleton')
        
        AgentHandler.start(self)
        
        
        
    def method(self, handle, methodName, args, subtypes, addr, userId):
        print "method"
        if not addr == self.qmfAddrVhsGateway:
            return
        
        try:
            print methodName
            if methodName == "test":
                handle.addReturnArgument("outstring", "an output string ...")
                self.agentSession.methodSuccess(handle)
            elif methodName == "vhsAttach":
                (type, vhsIP, vhsUdpPort, vhsTunnelID, vhsSessionID) = \
                                self.vhsgw.vhsAttach(
                                     args['type'],
                                     args['cpeTunnelID'],
                                     args['cpeSessionID'],
                                     args['cpeIP'],
                                     args['cpeUdpPort'])
                handle.addReturnArgument("vhsIP", vhsIP)
                handle.addReturnArgument("vhsUdpPort", vhsUdpPort)
                handle.addReturnArgument("vhsTunnelID", vhsTunnelID)
                handle.addReturnArgument("vhsSessionID", vhsSessionID)
                self.agentSession.methodSuccess(handle)
            elif methodName == "vhsDetach":
                self.vhsgw.vhsDetach(
                         args['type'],
                         args['cpeTunnelID'],
                         args['cpeSessionID'])
                self.agentSession.methodSuccess(handle)
            else:
                self.agentSession.raiseException(handle, "blubber")
        except:
            self.agentSession.raiseException(handle, "something failed, but we do not know, what ...")
            raise
  


class VhsGatewayEvent(object):
    """Base class for all events generated within class VhsGateway"""
    def __init__(self, source, type, opaque=''):
        self.source = source
        self.type = type
        self.opaque = opaque
        #print self
    
    def __str__(self, *args, **kwargs):
        return '<VhsGatewayEvent [type:' + str(self.type) + '] ' + object.__str__(self, *args, **kwargs) + ' >'



#class VhsGateway(Thread):
class VhsGateway(object):
    """
    main class for a Virtual Home Switch Gateway
    """
    def __init__(self, **kwargs):
        self.events = []
        (self.pipein, self.pipeout) = os.pipe()
        if 'brokerUrl' in kwargs:
            self.brokerUrl = kwargs['brokerUrl']
        else:
            self.brokerUrl = '127.0.0.1'
        try:
            self.qmfConnection = cqpid.Connection(self.brokerUrl)
            self.qmfConnection.open()
            self.qmfAgentSession = AgentSession(self.qmfConnection, "{interval:10}")
            self.qmfAgentSession.setVendor('bisdn.de')
            self.qmfAgentSession.setProduct('vhsGateway')
            self.qmfAgentSession.open()
            self.qmfAgentHandler = VhsGatewayQmfAgentHandler(self, self.qmfAgentSession)
        except:
            raise
        self.l2tpTunnels = []
        self.vhsIP = '1000::100'
        #Thread.__init__(self)
        #self.start()
        

    def __cleanup(self):
        pass

    
    def add_event(self, event):
        self.events.append(event)
        os.write(self.pipeout, str(event.type))


    def run(self):
        self.ep = select.epoll()
        self.ep.register(self.pipein, select.EPOLLET | select.EPOLLIN)
        
        while True:
            try :
                print "1"
                self.ep.poll(timeout=-1, maxevents=1)
                evType = os.read(self.pipein, 256)
                print "2"
                for event in self.events:
                    if event.type == EVENT_QUIT:
                        self.qmfAgentHandler.cancel()
                        return
                else:
                    self.events = []
            except KeyboardInterrupt:
                self.__cleanup()
                self.add_event(VhsGatewayEvent(self, EVENT_QUIT))


    def vhsAttach(self, type, cpeTunnelID, cpeSessionID, cpeIP, cpeUdpPort):
        vhsTunnelID = cpeTunnelID
        vhsIP = self.vhsIP
        vhsUdpPort = int(cpeUdpPort) + 1
        for l2tpTunnel in self.l2tpTunnels:
            if l2tpTunnel.peer_tunnel_id == cpeTunnelID:
                break
        else:
            l2tpTunnel = L2tpTunnel(vhsTunnelID, cpeTunnelID, vhsIP, cpeIP, vhsUdpPort, cpeUdpPort)
            self.l2tpTunnels.append(l2tpTunnel)
            
        print 'vhsAttach => ' + str(l2tpTunnel)

        vhsSessionID = cpeSessionID        
        for l2tpSession in l2tpTunnel.sessions:
            if l2tpSession.session_id == cpeSessionID:
                break
        else:
            name = 'l2tpeth' + str(vhsSessionID)
            l2tpTunnel.addSession(name, vhsSessionID, cpeSessionID)
                    
        return ('l2tp', vhsIP, vhsUdpPort, vhsTunnelID, vhsSessionID)


    def vhsDetach(self, type, vhsTunnelID, vhsSessionID):
        for l2tpTunnel in self.l2tpTunnels:
            if l2tpTunnel.tunnel_id == vhsTunnelID:
                for session_id, l2tpSession in l2tpTunnel.sessions.iteritems():
                    if l2tpSession.session_id == vhsSessionID:
                        l2tpTunnel.delSession(vhsSessionID)
                        break
    

if __name__ == "__main__":
    VhsGateway(brokerUrl=brokerUrl).run()
    #vhsGw = VhsGateway(brokerUrl=brokerUrl)

    
                 