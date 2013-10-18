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
import radvd
import routerlink
import datapath
import time
import qmf2
import cqmf2
import cqpid
import l2tp

EVENT_QUIT = 0

class VhsGatewayQmfAgentHandler(qmf2.AgentHandler):
    """
    QMF agent handler for class VhsGateway
    """
    def __init__(self, vhsgw, agentSession):
        qmf2.AgentHandler.__init__(self, agentSession)
        self.agentSession = agentSession
        
        self.qmfSchemaVhsGateway = qmf2.Schema(qmf2.SCHEMA_TYPE_DATA, "de.bisdn.proact.vhsgw", "vhsgw")
        #self.qmfSchemaVhsGateway.addProperty(qmf2.SchemaProperty("dptcoreid", qmf2.SCHEMA_DATA_INT))
        
        self.qmfSchemaVhsGateway_testMethod = qmf2.SchemaMethod("test", desc='testing method for QMF agent support in python')
        self.qmfSchemaVhsGateway_testMethod.addArgument(qmf2.SchemaProperty("subcmd", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaVhsGateway_testMethod.addArgument(qmf2.SchemaProperty("outstring", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_OUT))
        self.qmfSchemaVhsGateway.addMethod(self.qmfSchemaVhsGateway_testMethod)

        # add further methods here ...

        self.agentSession.registerSchema(self.qmfSchemaVhsGateway)

        self.vhsgw = vhsgw
        self.qmfDataVhsGateway = qmf2.Data(self.qmfSchemaVhsGateway)
        self.qmfAddrVhsGateway = self.agentSession.addData(self.qmfDataVhsGateway, 'vhsgw')
        
        
    def method(self, handle, methodName, args, subtypes, addr, userId):
        try:
            if methodName == "test":
                handle.addReturnArgument("outstring", "an output string ...")
                self.agentSession.methodSuccess(handle)
        except:
            self.agentSession.raiseException(handle, "something failed, but we do not know, what ...")
  


class VhsGatewayEvent(object):
    """Base class for all events generated within class VhsGateway"""
    def __init__(self, source, type, opaque=''):
        self.source = source
        self.type = type
        self.opaque = opaque
        #print self
    
    def __str__(self, *args, **kwargs):
        return '<VhsGatewayEvent [type:' + str(self.type) + '] ' + object.__str__(self, *args, **kwargs) + ' >'



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
            self.qmfAgentSession = qmf2.AgentSession(self.qmfConnection, "{interval:10, reconnect:True}")
            self.qmfAgentSession.setVendor('bisdn.de')
            self.qmfAgentSession.setProduct('vhsgw')
            self.qmfAgentSession.open()
            self.qmfAgentHandler = VhsGatewayQmfAgentHandler(self, self.qmfAgentSession)
        except:
            raise

    
    def add_event(self, event):
        #if not isinstance(event, HomeGatewayEvent):
        #    print "trying to add non-event, ignoring"
        #    print str(event)
        #    return
        self.events.append(event)
        os.write(self.pipeout, str(event.type))


    def run(self):
        self.ep = select.epoll()
        self.ep.register(self.pipein, select.EPOLLET | select.EPOLLIN)
        
        while True:
            try :
                self.ep.poll(timeout=-1, maxevents=1)
                evType = os.read(self.pipein, 256) 
                for event in self.events:
                    if event.type == EVENT_QUIT:
                        return
                else:
                    self.events = []
            except KeyboardInterrupt:
                self.__cleanup()
                self.add_event(VhsGatewayEvent(self, EVENT_QUIT))
            
    
    

if __name__ == "__main__":
    VhsGateway(brokerUrl=brokerUrl).run()

    
                 