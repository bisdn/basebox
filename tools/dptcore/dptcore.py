#!/usr/bin/python

import select
import qmf2
import cqmf2
import cqpid
import sys
import os

EVENT_QUIT = 0


    
class DptCoreQmfAgentHandler(qmf2.AgentHandler):
    def __init__(self, agentSession):
        self.agentSession = agentSession
        self.__set_schema()
        qmf2.AgentHandler.__init__(self, agentSession)

        self.dptcore = {}
        self.dptcore['data'] = qmf2.Data(self.sch_node)
        self.dptcore['addr'] = self.agentSession.addData(self.dptcore['data'], 'dptcore')

        
    def __set_schema(self):
        self.sch_node = qmf2.Schema(qmf2.SCHEMA_TYPE_DATA, "de.bisdn.homegw", "dptcore")
        #self.sch_node.addProperty(qmf2.SchemaProperty("dptcoreid", qmf2.SCHEMA_DATA_INT))
        self.agentSession.registerSchema(self.sch_node)
        print self.sch_node
        
        
    def method(self, handle, methodName, args, subtypes, addr, userId):
        print methodName
        pass
        #...method handling code goes here...
        #For success, add output arguments:
        #    handle.addReturnArgument("argname", argvalue)
        #    ...
        #    self.agent.methodSuccess(handle)
        #For failure, raise an exception:
        #    self.agent.raiseException(handle, "error text")
        #Or, if you have created a schema for a structured exception:
        #    ex = qmf2.Data(exceptionSchema)
        #    ex.whatHappened = "it failed"
        #    ex.howBad = 84
        #    ex.detailMap = {}
        #    ...
        #    self.agent.raiseException(handle, ex)



class DptCoreException(Exception):
    pass


class DptCoreEvent(object):
    def __init__(self, source, type):
        self.source = source
        self.type = type


class DptCore(object):
    def __init__(self, broker_url="localhost"):
        (self.pipein, self.pipeout) = os.pipe()
        self.events = []
        self.qmf    = {}
        try:
            self.qmf['connection'] = cqpid.Connection(broker_url)
            self.qmf['connection'].open()
            self.qmf['session'] = qmf2.AgentSession(self.qmf['connection'], "{interval:10, reconnect:True}")
            self.qmf['session'].setVendor('bisdn.de')
            self.qmf['session'].setProduct('homegw')
            self.qmf['session'].open()
            self.qmf['handler'] = DptCoreQmfAgentHandler(self.qmf['session'])
        except:
            raise
        



    def add_event(self, event):
        self.events.append(event)
        os.write(self.pipeout, str(event.type))

    
    def run(self):
        self.ep = select.epoll()
        self.ep.register(self.pipein, select.EPOLLET | select.EPOLLIN)
                
        while True:
            try :
                self.ep.poll(timeout=-1, maxevents=1)
                evType = os.read(self.pipein, 256)
                #print "epoll done: " + evType + "\n"
                
                for event in self.events:
                    if event.type == EVENT_QUIT:
                        return
                else:
                    self.events = []
            except KeyboardInterrupt:
                self.add_event(DptCoreEvent(self, EVENT_QUIT))




if __name__ == "__main__":
    DptCore().run()

    
