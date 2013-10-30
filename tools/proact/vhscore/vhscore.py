#!/usr/bin/python

import sys
sys.path.append('../..')
print sys.path

import proact.common.basecore
import proact.dptcore.dptcore
import ConfigParser
import qmf2



class VhsCoreQmfAgentHandler(proact.common.basecore.BaseCoreQmfAgentHandler):
    def __init__(self, vhsCore, agentSession):
        proact.common.basecore.BaseCoreQmfAgentHandler.__init__(self, agentSession)
        self.vhsCore = vhsCore
        self.qmfVhsCore = {}
        self.qmfVhsCore['data'] = qmf2.Data(self.qmfSchemaVhsCore)
        self.qmfVhsCore['data'].vhsCoreID = vhsCore.vhsCoreID
        self.qmfVhsCore['addr'] = self.agentSess.addData(self.qmfVhsCore['data'], 'vhscore')
        
    def setSchema(self):
        self.qmfSchemaVhsCore = qmf2.Schema(qmf2.SCHEMA_TYPE_DATA, "de.bisdn.proact", "vhscore")
        self.qmfSchemaVhsCore.addProperty(qmf2.SchemaProperty("vhsCoreID", qmf2.SCHEMA_DATA_STRING))

        # test method
        #        
        self.qmfSchemaMethodTest = qmf2.SchemaMethod("test", desc='testing method for QMF agent support in python')
        self.qmfSchemaMethodTest.addArgument(qmf2.SchemaProperty("instring", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodTest.addArgument(qmf2.SchemaProperty("outstring", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_OUT))
        self.qmfSchemaVhsCore.addMethod(self.qmfSchemaMethodTest)

        # UserSessionAdd method
        #
        self.qmfSchemaMethodUserSessionAdd = qmf2.SchemaMethod("userSessionAdd", desc='add a new authenticated user session')
        self.qmfSchemaMethodUserSessionAdd.addArgument(qmf2.SchemaProperty('userIdentity', qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodUserSessionAdd.addArgument(qmf2.SchemaProperty('ipAddress', qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodUserSessionAdd.addArgument(qmf2.SchemaProperty('validLifetime', qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaVhsCore.addMethod(self.qmfSchemaMethodUserSessionAdd)

        # UserSessionDel method
        #
        self.qmfSchemaMethodUserSessionDel = qmf2.SchemaMethod("userSessionDel", desc='delete an authenticated user session')
        self.qmfSchemaMethodUserSessionDel.addArgument(qmf2.SchemaProperty('userIdentity', qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodUserSessionDel.addArgument(qmf2.SchemaProperty('ipAddress', qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaVhsCore.addMethod(self.qmfSchemaMethodUserSessionDel)


        self.agentSess.registerSchema(self.qmfSchemaVhsCore)
           
        
    def method(self, handle, methodName, args, subtypes, addr, userId):
        try:
            print "method: " + str(methodName)
            if methodName == "test":
                handle.addReturnArgument("outstring", 'you sent <' + args['instring'] + '>')
                self.agentSess.methodSuccess(handle)
            elif methodName == 'userSessionAdd':
                try:
                    self.vhsCore.userSessionAdd(args['userIdentity'], args['ipAddress'], args['validLifetime'])
                    self.agentSess.methodSuccess(handle)
                except:
                    self.agentSess.raiseException(handle, "call for userSessionAdd() failed for userID " + args['userIdentity'] + ' and ipAddress ' + args['ipAddress'])
                
            elif methodName == 'userSessionDel':
                try:
                    self.vhsCore.userSessionDel(args['userIdentity'], args['ipAddress'])
                    self.agentSess.methodSuccess(handle)
                except:
                    self.agentSess.raiseException(handle, "call for userSessionDel() failed for userID " + args['userIdentity'])
            
            else:
                self.agentSess.raiseException(handle, 'method ' + methodName + ' not found')
        except:
            print "something failed, but we do not know, what ..."
            self.agentSess.raiseException(handle, "something failed, but we do not know, what ...")



class VhsCore(proact.common.basecore.BaseCore):
    """
    a description would be useful here
    """
    def __init__(self, **kwargs):
        self.parseConfig()
        self.brokerUrl = brokerUrl
        self.vendor = kwargs.get('vendor', 'bisdn.de')
        self.product = kwargs.get('product', 'vhscore')
        proact.common.basecore.BaseCore.__init__(self, vendor=self.vendor, product=self.product)
        self.agentHandler = VhsCoreQmfAgentHandler(self, self.qmfAgent.agentSess)
        self.initVhsXdpd()
        self.initVhsDptCore()
        
    def cleanUp(self):
        self.agentHandler.cancel()
        self.xdpdVhsProxy.destroyLsi(self.dpid1)
        self.xdpdVhsProxy.destroyLsi(self.dpid0)
        self.dptCoreProxy.resetL2tpState()
        
    def userSessionAdd(self, userIdentity, ipAddress, validLifetime):
        print 'adding user session for user ' + userIdentity + ' on IP address ' + ipAddress + ' with lifetime ' + validLifetime 

    def userSessionDel(self, userIdentity, ipAddress):
        print 'deleting user session for user ' + userIdentity + ' on IP address ' + ipAddress 

    def handleEvent(self, event):
        pass
    
    def parseConfig(self):
        self.config = ConfigParser.ConfigParser()
        self.config.read('vhscore.conf')
        self.brokerUrl = self.config.get('vhscore', 'BROKERURL', '127.0.0.1')
        self.vhsDptCoreID = self.config.get('dptcore', 'DPTCOREID', 'vhs-dptcore-0')

    
    def initVhsXdpd(self):
        """
        1. check for LSIs and create them, if not found
        2. create virtual link between both LSIs
        """
        self.vhsDptXdpdID = self.config.get('xdpd', 'XDPDID', 'vhs-xdpd-0')
        self.xdpdVhsProxy = proact.common.xdpdproxy.XdpdProxy(self.brokerUrl, self.vhsDptXdpdID)
        lsiNames = self.config.get('xdpd', 'LSIS').split()
        for lsiName in lsiNames:
            try:
                dpid        = self.config.get(lsiName, 'DPID')
                dpname      = self.config.get(lsiName, 'DPNAME')
                ofversion   = self.config.get(lsiName, 'OFVERSION')
                ntables     = self.config.get(lsiName, 'NTABLES')
                ctlaf       = self.config.get(lsiName, 'CTLAF')
                ctladdr     = self.config.get(lsiName, 'CTLADDR')
                ctlport     = self.config.get(lsiName, 'CTLPORT')
                reconnect   = self.config.get(lsiName, 'RECONNECT')
                self.xdpdVhsProxy.createLsi(dpname, dpid, ofversion, ntables, ctlaf, ctladdr, ctlport, reconnect)    
            except ConfigParser.NoOptionError:
                print 'option not found for LSI ' + str(lsiName) + ', continuing with next LSI'
            except ConfigParser.NoSectionError:
                print 'section not found for LSI ' + str(lsiName) +  ', continuing with next LSI'
                
        virtLinks = self.config.get('xdpd', 'VIRTLINKS').split()
        for virtLink in virtLinks:
            try:
                dpid1       = self.config.get(virtLink, 'DPID1')
                dpid2       = self.config.get(virtLink, 'DPID2')
                [devname1, devname2] = self.xdpdVhsProxy.createVirtualLink(dpid1, dpid2)
                print devname1
                print devname2
            except ConfigParser.NoOptionError:
                print 'option not found for virtual link ' + str(virtLink) + ', continuing with next virtual link'
            except ConfigParser.NoSectionError:
                print 'section not found for virtual link ' + str(virtLink) + ', continuing with next virtual link'


            
        
    def initVhsDptCore(self):
        """
        1. create virtual ethernet cable (veth pair)
        
        """
        self.dptCoreProxy = proact.dptcore.dptcore.DptCoreProxy(self.brokerUrl, self.vhsDptCoreID)
        self.dptCoreProxy.addVethLink('veth0', 'veth1')
        
    

if __name__ == "__main__":
    VhsCore().run() 
    
    