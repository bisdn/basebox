#!/usr/bin/python

from qmf.console import Session


class QmfConsoleException(Exception):
    """
    Basic exception for class QmfConsole
    """
    pass


class QmfConsole(object):
    """
    Simple Abstraction to send QMF console commands
    """
    def __init__(self, broker_url = "amqp://localhost:5672"):
        try:
            self.session = Session()
            self.broker = self.session.addBroker(broker_url)
        except:
            print "Connection failed"
            raise


    def __del__(self):
        self.session.close()
        
        
    def getObjects(self, **kwargs):
        return self.session.getObjects(_class = kwargs['_class'], _package = kwargs['_package'])
        
        
        
        
        