#!/usr/bin/python

import sys
sys.path.append('../..')

import proact.common.basecore
import proact.common.xdpdproxy
import proact.common.dptcore
import ConfigParser
import time
import qmf2



class HgwCoreQmfAgentHandler(proact.common.basecore.BaseCoreQmfAgentHandler):
    def __init__(self, hgwCore, agentSession):
        proact.common.basecore.BaseCoreQmfAgentHandler.__init__(self, agentSession)
        self.hgwCore = hgwCore
        self.qmfHgwCore = {}
        self.qmfHgwCore['data'] = qmf2.Data(self.qmfSchemaHgwCore)
        self.qmfHgwCore['data'].hgwCoreID = hgwCore.hgwCoreID
        self.qmfHgwCore['addr'] = self.agentSess.addData(self.qmfHgwCore['data'], 'hgwcore')
        print 'registering on QMF with hgwCoreID ' + hgwCore.hgwCoreID
        
    def setSchema(self):
        self.qmfSchemaHgwCore = qmf2.Schema(qmf2.SCHEMA_TYPE_DATA, "de.bisdn.proact", "hgwcore")
        self.qmfSchemaHgwCore.addProperty(qmf2.SchemaProperty("hgwCoreID", qmf2.SCHEMA_DATA_STRING))

        # test method
        #        
        self.qmfSchemaMethodTest = qmf2.SchemaMethod("test", desc='testing method for QMF agent support in python')
        self.qmfSchemaMethodTest.addArgument(qmf2.SchemaProperty("instring", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodTest.addArgument(qmf2.SchemaProperty("outstring", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_OUT))
        self.qmfSchemaHgwCore.addMethod(self.qmfSchemaMethodTest)

        # UserSessionAdd method
        #
        self.qmfSchemaMethodUserSessionAdd = qmf2.SchemaMethod("userSessionAdd", desc='add a new authenticated user session')
        self.qmfSchemaMethodUserSessionAdd.addArgument(qmf2.SchemaProperty('userIdentity', qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodUserSessionAdd.addArgument(qmf2.SchemaProperty('ipAddress', qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodUserSessionAdd.addArgument(qmf2.SchemaProperty('validLifetime', qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaHgwCore.addMethod(self.qmfSchemaMethodUserSessionAdd)

        # UserSessionDel method
        #
        self.qmfSchemaMethodUserSessionDel = qmf2.SchemaMethod("userSessionDel", desc='delete an authenticated user session')
        self.qmfSchemaMethodUserSessionDel.addArgument(qmf2.SchemaProperty('userIdentity', qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodUserSessionDel.addArgument(qmf2.SchemaProperty('ipAddress', qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaHgwCore.addMethod(self.qmfSchemaMethodUserSessionDel)


        self.agentSess.registerSchema(self.qmfSchemaHgwCore)
           
        
    def method(self, handle, methodName, args, subtypes, addr, userId):
        try:
            print "method: " + str(methodName)
            if methodName == "test":
                handle.addReturnArgument("outstring", 'you sent <' + args['instring'] + '>')
                self.agentSess.methodSuccess(handle)
            elif methodName == 'userSessionAdd':
                try:
                    self.HgwCore.userSessionAdd(args['userIdentity'], args['ipAddress'], args['validLifetime'])
                    self.agentSess.methodSuccess(handle)
                except:
                    self.agentSess.raiseException(handle, "call for userSessionAdd() failed for userID " + args['userIdentity'] + ' and ipAddress ' + args['ipAddress'])
                
            elif methodName == 'userSessionDel':
                try:
                    self.HgwCore.userSessionDel(args['userIdentity'], args['ipAddress'])
                    self.agentSess.methodSuccess(handle)
                except:
                    self.agentSess.raiseException(handle, "call for userSessionDel() failed for userID " + args['userIdentity'])
            
            else:
                self.agentSess.raiseException(handle, 'method ' + methodName + ' not found')
        except:
            print "something failed, but we do not know, what ..."
            self.agentSess.raiseException(handle, "something failed, but we do not know, what ...")





class HgwCore(proact.common.basecore.BaseCore):
    """
    a description would be useful here
    """
    def __init__(self, **kwargs):
        try:
            self.parseConfig()
            self.vendor = kwargs.get('vendor', 'bisdn.de')
            self.product = kwargs.get('product', 'hgwcore')
            self.brokerUrl = kwargs('brokerUrl', '127.0.0.1')
            proact.common.basecore.BaseCore.__init__(self, self.brokerUrl, vendor=self.vendor, product=self.product)
            self.agentHandler = HgwCoreQmfAgentHandler(self, self.qmfAgent.agentSess)
            self.initHgwXdpd()
            self.initHgwDptCore()
        except Exception, e:
            print 'HgwCore::init() failed ' + str(e)
            self.cleanUp()
        
    def cleanUp(self):
        print 'clean-up started ...'
        try:
            self.agentHandler.cancel()
        except:
            pass

                    
        
    def userSessionAdd(self, userIdentity, ipAddress, validLifetime):
        print 'adding user session for user ' + userIdentity + ' on IP address ' + ipAddress + ' with lifetime ' + validLifetime 

    def userSessionDel(self, userIdentity, ipAddress):
        print 'deleting user session for user ' + userIdentity + ' on IP address ' + ipAddress 

    def handleEvent(self, event):
        pass
    
    def handleNewLink(self, ifindex, devname, ndmsg):
        print 'NewLink (' + str(ifindex) + ',' + str(devname) + ')'
    
    def handleDelLink(self, ifindex, devname, ndmsg):
        print 'DelLink (' + str(ifindex) + ',' + str(devname) + ')'
    
    def handleNewAddr(self, ifindex, family, addr, prefixlen, scope, ndmsg):
        print 'NewAddr (' + str(ifindex) + ',' + str(family) + ',' + str(addr) + ',' + str(prefixlen) + ',' + str(scope) + ')'
        
    def handleDelAddr(self, ifindex, family, addr, prefixlen, scope, ndmsg):
        print 'DelAddr (' + str(ifindex) + ',' + str(family) + ',' + str(addr) + ',' + str(prefixlen) + ',' + str(scope) + ')'
    
    def handleNewRoute(self, family, dst, dstlen, table, type, scope, ndmsg):
        print 'NewRoute (' + str(family) + ',' + str(dst) + ',' + str(dstlen) + ',' + str(table) + ',' + str(type) + ',' + str(scope) + ')'
    
    def handleDelRoute(self, family, dst, dstlen, table, type, scope, ndmsg):
        print 'DelRoute (' + str(family) + ',' + str(dst) + ',' + str(dstlen) + ',' + str(table) + ',' + str(type) + ',' + str(scope) + ')'
        
    def parseConfig(self):
        self.config = ConfigParser.ConfigParser()
        self.config.read('hgwcore.conf')
        self.brokerUrl = self.config.get('hgwcore', 'BROKERURL', '127.0.0.1')
        self.hgwCoreID = self.config.get('hgwcore', 'HGWCOREID', 'hgw-core-0')
        self.hgwDptCoreID = self.config.get('dptcore', 'DPTCOREID', 'hgw-dptcore-0')

    
    def initHgwXdpd(self):
        """
        1. check for LSIs and create them, if not found
        2. create virtual link between both LSIs
        """
        try:
            self.hgwDptXdpdID = self.config.get('xdpd', 'XDPDID', 'hgw-xdpd-0')
            self.xdpdHgwProxy = proact.common.xdpdproxy.XdpdProxy(self.brokerUrl, self.hgwDptXdpdID)
        except proact.common.xdpdproxy.XdpdProxyException, e:
            print 'HgwCore::initHgwXdpd() initializing xdpd failed ' +  str(e)
            self.addEvent(proact.common.basecore.BaseCoreEvent(self, self.EVENT_QUIT))

            
        
    def initHgwDptCore(self):
        """
        1. create virtual ethernet cable (veth pair)
        
        """
        try:
            self.dptCoreProxy = proact.common.dptcore.DptCoreProxy(self.brokerUrl, self.hgwDptCoreID)
            self.dptCoreProxy.reset()
        except proact.common.dptcore.DptCoreProxyException, e:
            print 'HgwCore::initHgwDptCore() initializing dptcore failed ' + str(e)
            self.addEvent(proact.common.basecore.BaseCoreEvent(self, self.EVENT_QUIT))
    

if __name__ == "__main__":
    HgwCore().run() 
    
