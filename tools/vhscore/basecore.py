#!/usr/bin/python

import qmfhelper
import select
import sys
import os


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
    a description would be useful here
    """
    EVENT_QUIT = 1
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
        
    def cleanUp(self):
        raise BaseCoreException('BaseCore::cleanUp() method not implemented')
            
    def addEvent(self, event):
        try:
            if not isinstance(event, BaseCoreEvent):
                raise BaseCoreException('event is not of type BaseCoreEvent')
            self.events.append(event)
            os.write(self.pipeout, str(event.type))
        except BaseCoreException, e:
            print 'ignoring event ' + str(e)
            

    def handleEvent(self, event):
        raise BaseCoreException('BaseCore::handleEvent() method not implemented')
    
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
                        print "done."
                        return
                    self.handleEvent(event)
                else:
                    self.events = []
            except KeyboardInterrupt:
                sys.stdout.write("terminating...")
                self.cleanUp()
                self.addEvent(BaseCoreEvent(self, self.EVENT_QUIT))



if __name__ == "__main__":
    BaseCore(brokerUrl="127.0.0.1").run() 
    
    
    