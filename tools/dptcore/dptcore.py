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
        qmf2.AgentHandler.__init__(self, agentSession)
        self.agentSession = agentSession
        self.__set_schema()
        self.dptcore = {}
        self.dptcore['data'] = qmf2.Data(self.sch_dptcore)
        self.dptcore['addr'] = self.agentSession.addData(self.dptcore['data'], 'dptcore')

        
    def __set_schema(self):
        self.sch_dptcore = qmf2.Schema(qmf2.SCHEMA_TYPE_DATA, "de.bisdn.homegw", "dptcore")
        #self.sch_dptcore.addProperty(qmf2.SchemaProperty("dptcoreid", qmf2.SCHEMA_DATA_INT))
        self.sch_dptcore_testMethod = qmf2.SchemaMethod("test", desc='testing method for QMF agent support in python')
        self.sch_dptcore_testMethod.addArgument(qmf2.SchemaProperty("subcmd", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_dptcore_testMethod.addArgument(qmf2.SchemaProperty("outstring", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_OUT))
        self.sch_dptcore.addMethod(self.sch_dptcore_testMethod)
        self.agentSession.registerSchema(self.sch_dptcore)
        print self.sch_dptcore
        
        
    def method(self, handle, methodName, args, subtypes, addr, userId):
        print methodName
        if methodName == "test":
            handle.addReturnArgument("outstring", "an output string ...")
            self.agentSession.methodSuccess(handle)
        elif methodName == "blub":
            self.agentSession.methodSuccess(handle)
        else:
            self.agentSession.raiseException(handle, "blubber")



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
        # start QMF agent handler thread 
        self.qmf['handler'].start()
        
        self.ep = select.epoll()
        self.ep.register(self.pipein, select.EPOLLET | select.EPOLLIN)
                
        cont = True
        while cont:
            try :
                self.ep.poll(timeout=-1, maxevents=1)
                evType = os.read(self.pipein, 256)
                                
                for event in self.events:
                    if event.type == EVENT_QUIT:
                        cont = False
                else:
                    self.events = []
            except KeyboardInterrupt:
                self.add_event(DptCoreEvent(self, EVENT_QUIT))

        # stop QMF agent handler thread 
        self.qmf['handler'].cancel()
        # close QMF AgentSession
        self.qmf['session'].close()
        # close QMF connection
        self.qmf['connection'].close()


if __name__ == "__main__":
    DptCore().run()

    
