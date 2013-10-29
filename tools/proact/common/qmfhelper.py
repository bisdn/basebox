#!/usr/bin/python

from qmf.console import Session
import qmf2
import cqmf2
import cqpid


class QmfException(Exception):
    """base exception for Qmf* classes"""
    def __init__(self, serror = ""):
        self.serror = serror
    def __str__(self):
        return '<QmfException: ' + self.serror + ' >'


class QmfConsoleException(QmfException):
    def __init__(self, serror):
        QmfException.__init__(self, serror)       
    def __str__(self):
        return '<QmfConsoleException: ' + self.serror + ' >'
    

class QmfAgentException(QmfException):
    def __init__(self, serror):
        QmfException.__init__(self, serror)       
    def __str__(self):
        return '<QmfAgentException: ' + self.serror + ' >'

    

class QmfConsole(object):
    """
    Simple Abstraction to send QMF console commands
    """
    def __init__(self, brokerUrl="amqp://localhost:5672", **kwargs):         
        try:
            self.consoleSession = Session()
            self.consoleBroker = self.consoleSession.addBroker(brokerUrl)
        except:
            print "Connection failed"
            raise QmfConsoleException('unable to connect to ' + brokerUrl)


    def __del__(self):
        self.consoleSession.close()
        
        
    def getObjects(self, **kwargs):
        if not 'class' in kwargs:
            raise QmfConsoleException('class not found')
        if not 'package' in kwargs:
            raise QmfConsoleException('package not found')
        return self.consoleSession.getObjects(_class = kwargs['class'], _package = kwargs['package'])
        
        
        
        
        
        
        
    
class QmfAgentHandler(qmf2.AgentHandler):
    """
    overload methods setSchema() and method()
    """
    def __init__(self, agentSession):
        qmf2.AgentHandler.__init__(self, agentSession)
        self.agentSession = agentSession
        self.setSchema()

        
    def setSchema(self):
        pass

        
    def method(self, handle, methodName, args, subtypes, addr, userId):
        """
        to be overwritten
        """
        self.agentSession.raiseException(handle, "method " + methodName + " not implemented")
        

   
   
        
        
        
        
        
class QmfAgent(object):
    
    def __init__(self, brokerUrl="127.0.0.1", **kwargs):     
        self.brokerUrl = brokerUrl       
        if 'vendor' in kwargs:
            self.vendor = kwargs['vendor']
        else:
            self.vendor = 'bisdn.de'

        if 'product' in kwargs:
            self.product = kwargs['product']
        else:
            self.product = 'product'

        try:
            self.agentConn = cqpid.Connection(self.brokerUrl)
            self.agentConn.open()
            self.agentSess = qmf2.AgentSession(self.agentConn, "{interval:10, reconnect:True}")
            self.agentSess.setVendor(self.vendor)
            self.agentSess.setProduct(self.product)
            self.agentSess.open()
            #self.agentHandler = QmfAgentHandler(self, self.agentSess)
        except:
            raise QmfAgentException('unable to connect to broker ' + self.brokerUrl)
        
        
        