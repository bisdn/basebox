#!/usr/bin/python

import sys
sys.path.append('../..')
print sys.path

import proact.common.basecore
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
    def __init__(self, vhsCoreID='vhscore-0', brokerUrl="127.0.0.1", **kwargs):
        self.vhsCoreID = vhsCoreID
        self.vendor = kwargs.get('vendor', 'bisdn.de')
        self.product = kwargs.get('product', 'vhscore')
        proact.common.basecore.BaseCore.__init__(self, vendor=self.vendor, product=self.product)
        self.agentHandler = VhsCoreQmfAgentHandler(self, self.qmfAgent.agentSess)
        
    def cleanUp(self):
        self.agentHandler.cancel()
        
    def userSessionAdd(self, userIdentity, ipAddress, validLifetime):
        print 'adding user session for user ' + userIdentity + ' on IP address ' + ipAddress + ' with lifetime ' + validLifetime 

    def userSessionDel(self, userIdentity, ipAddress):
        print 'deleting user session for user ' + userIdentity + ' on IP address ' + ipAddress 

    def handleEvent(self, event):
        pass
    
    def setupDatapath(self, xdpdID = 'vhs-xdpd-0'):
        """
        1. check for LSIs and create, if not found
        2. create virtual link between both LSIs
        """
        try:
            dptQmfHandles = self.qmfConsole.getObjects(_class='xdpd', _package='de.bisdn.xdpd')
            for dptQmfHandle in dptQmfhandles:
                if dptQmfHandle.xdpdID == xdpdID:
                    break
            else:
                print 'no xdpd instance found'
                return
            

            
                lsiCreateMethod.addArgument(qmf::SchemaProperty("dpid",     qmf::SCHEMA_DATA_INT,         "{dir:INOUT}"));
    lsiCreateMethod.addArgument(qmf::SchemaProperty("dpname",     qmf::SCHEMA_DATA_STRING,     "{dir:IN}"));
    lsiCreateMethod.addArgument(qmf::SchemaProperty("ofversion",qmf::SCHEMA_DATA_INT,         "{dir:IN}"));
    lsiCreateMethod.addArgument(qmf::SchemaProperty("ntables",     qmf::SCHEMA_DATA_INT,         "{dir:IN}"));
    lsiCreateMethod.addArgument(qmf::SchemaProperty("ctlaf",    qmf::SCHEMA_DATA_INT,         "{dir:IN}"));
    lsiCreateMethod.addArgument(qmf::SchemaProperty("ctladdr",     qmf::SCHEMA_DATA_STRING,     "{dir:IN}"));
    lsiCreateMethod.addArgument(qmf::SchemaProperty("ctlport",     qmf::SCHEMA_DATA_INT,         "{dir:IN}"));
    lsiCreateMethod.addArgument(qmf::SchemaProperty("reconnect",qmf::SCHEMA_DATA_INT,         "{dir:IN}")); 
            
        except ListError, e:
            print 'no xdpd instance found'
        


if __name__ == "__main__":
    VhsCore('vhscore-0', '127.0.0.1').run() 
    
    