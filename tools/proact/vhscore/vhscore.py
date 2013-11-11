#!/usr/bin/python

import sys
sys.path.append('../..')
#print sys.path

import proact.common.basecore
import proact.common.xdpdproxy
import proact.common.dptcore
import ConfigParser
import time
import qmf2



class VhsCoreQmfAgentHandler(proact.common.basecore.BaseCoreQmfAgentHandler):
    def __init__(self, vhsCore, agentSession):
        proact.common.basecore.BaseCoreQmfAgentHandler.__init__(self, agentSession)
        self.vhsCore = vhsCore
        self.qmfVhsCore = {}
        self.qmfVhsCore['data'] = qmf2.Data(self.qmfSchemaVhsCore)
        self.qmfVhsCore['data'].vhsCoreID = vhsCore.vhsCoreID
        self.qmfVhsCore['addr'] = self.agentSess.addData(self.qmfVhsCore['data'], 'vhscore')
        print 'registering on QMF with vhsCoreID ' + vhsCore.vhsCoreID
        
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
        try:
            self.linkNames = {'wan':[], 'dmz':[], 'lan':[] }
            self.dmzLinks = {}
            self.lanLinks = {}
            self.wanLinks = {}
            
            self.conffile = kwargs.get('conffile', 'vhscore.conf')
            self.parseConfig()
            self.vendor = kwargs.get('vendor', 'bisdn.de')
            self.product = kwargs.get('product', 'vhscore')
            #self.brokerUrl = kwargs('brokerUrl', '127.0.0.1')
            
            #self.linkNames = {'dmz':kwargs.get('dmzLinks', []), 'lan':kwargs.get('lanLinks', []), 'wan':kwargs.get('wanLinks', [])}
            #print self.linkNames
            
                    
            proact.common.basecore.BaseCore.__init__(self, self.brokerUrl, vendor=self.vendor, product=self.product)
            self.agentHandler = VhsCoreQmfAgentHandler(self, self.qmfAgent.agentSess)
            self.initVhsXdpd()
            self.initVhsDptCore()
        except Exception, e:
            print 'VhsCore::init() failed ' + str(e)
            self.cleanUp()
        
    def cleanUp(self):
        print 'clean-up started ...'
        try:
            self.agentHandler.cancel()
        except:
            pass
        for devname in self.dmzLinks:
            self.dmzLinks[devname].disable()
        self.dmzLinks = {}
        for devname in self.lanLinks:
            self.lanLinks[devname].disable()
        self.lanLinks = {}
        for devname in self.wanLinks:
            self.wanLinks[devname].disable()
        self.wanLinks = {}
        
    def userSessionAdd(self, userIdentity, ipAddress, validLifetime):
        print 'adding user session for user ' + userIdentity + ' on IP address ' + ipAddress + ' with lifetime ' + validLifetime 

    def userSessionDel(self, userIdentity, ipAddress):
        print 'deleting user session for user ' + userIdentity + ' on IP address ' + ipAddress 

    def handleEvent(self, event):
        if event.type == proact.common.basecore.BaseCore.EVENT_NEW_LINK:
            print 'NewLink (' + str(event.source) + ')'
            link = event.source
            if link.devname in self.linkNames['dmz']:
                link.linkType = 'dmz'
                link.disable()
                link.enable(0)
                self.dmzLinks[link.devname] = link
            elif link.devname in self.linkNames['lan']:
                link.linkType = 'lan'
                link.disable()
                link.enable(0)
                self.lanLinks[link.devname] = link
            elif link.devname in self.linkNames['wan']:
                link.linkType = 'wan'
                link.disable()
                link.enable(2)                
                self.wanLinks[link.devname] = link
                
        elif event.type == proact.common.basecore.BaseCore.EVENT_DEL_LINK:
            print 'DelLink (' + str(event.source) + ')'
            link = event.source
            if link.devname in self.dmzLinks:
                self.dmzLinks.pop(link.devname, None)
            if link.devname in self.lanLinks:
                self.lanLinks.pop(link.devname, None)
            if link.devname in self.wanLinks:
                self.wanLinks.pop(link.devname, None)
            
        elif event.type == proact.common.basecore.BaseCore.EVENT_NEW_ADDR:
            print 'NewAddr (' + str(event.source) + ')'
        elif event.type == proact.common.basecore.BaseCore.EVENT_DEL_ADDR:
            print 'DelAddr (' + str(event.source) + ')'
        elif event.type == proact.common.basecore.BaseCore.EVENT_NEW_ROUTE:
            print 'NewRoute (' + str(event.source) + ')'
        elif event.type == proact.common.basecore.BaseCore.EVENT_DEL_ROUTE:
            print 'DelRoute (' + str(event.source) + ')'
        elif event.type == proact.common.basecore.BaseCore.EVENT_LINK_UP:
            print 'LinkUp (' + str(event.source) + ')'
        elif event.type == proact.common.basecore.BaseCore.EVENT_LINK_DOWN:
            print 'LinkDown (' + str(event.source) + ')'
        elif event.type == proact.common.basecore.BaseCore.EVENT_RA_ATTACHED:
            print 'RA-Attached (' + str(event.source) + ')'
            link = event.source
            if link.devname in self.wanLinks:
                link.dhclient.sendRequest()
            
        elif event.type == proact.common.basecore.BaseCore.EVENT_RA_DETACHED:
            print 'RA-Detached (' + str(event.source) + ')'
            link = event.source
            if link.devname in self.wanLinks:
                link.dhclient.killClient()            
            
        elif event.type == proact.common.basecore.BaseCore.EVENT_PREFIX_ATTACHED:
            print 'Prefix-Attached (' + str(event.source) + ')'
            dhclient = event.source
            for devname, lanLink in self.lanLinks.iteritems():
                for prefix in dhclient.new_prefixes:
                    p = prefix.get_subprefix(lanLink.uniquePrefix).prefix
                    lanLink.addIPv6Addr(str(p)+'1', 64)
                for prefix in dhclient.new_prefixes:
                    p = prefix.get_subprefix(lanLink.uniquePrefix).prefix
                    lanLink.radvd.addPrefix(proact.common.ipv6prefix.IPv6Prefix(str(p), 64))
                lanLink.radvd.restart()
                    
            for devname, dmzLink in self.dmzLinks.iteritems():
                for prefix in dhclient.new_prefixes:
                    p = prefix.get_subprefix(dmzLink.uniquePrefix).prefix
                    dmzLink.addIPv6Addr(str(p)+'1', 64)
                for prefix in dhclient.new_prefixes:
                    p = prefix.get_subprefix(dmzLink.uniquePrefix).prefix
                    dmzLink.radvd.addPrefix(proact.common.ipv6prefix.IPv6Prefix(str(p), 64))
                dmzLink.radvd.restart()
        
        elif event.type == proact.common.basecore.BaseCore.EVENT_PREFIX_DETACHED:
            print 'Prefix-Detached (' + str(event.source) + ')'
            dhclient = event.source
            for devname, lanLink in self.lanLinks.iteritems():
                for prefix in dhclient.new_prefixes:
                    p = prefix.get_subprefix(lanLink.uniquePrefix).prefix
                    lanLink.radvd.delPrefix(proact.common.ipv6prefix.IPv6Prefix(str(p), 64))
                lanLink.radvd.restart()
                for prefix in dhclient.new_prefixes:
                    p = prefix.get_subprefix(lanLink.uniquePrefix).prefix
                    lanLink.delIPv6Addr(str(p)+'1', 64)
            
            for devname, dmzLink in self.dmzLinks.iteritems():
                for prefix in dhclient.new_prefixes:
                    p = prefix.get_subprefix(dmzLink.uniquePrefix).prefix
                    dmzLink.radvd.delPrefix(proact.common.ipv6prefix.IPv6Prefix(str(p), 64))
                dmzLink.radvd.restart()
                for prefix in dhclient.new_prefixes:
                    p = prefix.get_subprefix(dmzLink.uniquePrefix).prefix
                    dmzLink.delIPv6Addr(str(p)+'1', 64)
                    
        elif event.type == proact.common.basecore.BaseCore.EVENT_RADVD_START:
            print 'RADVD-Start (' + str(event.source) + ')'
        elif event.type == proact.common.basecore.BaseCore.EVENT_RADVD_STOP:
            print 'RADVD-Stop (' + str(event.source) + ')'
                                    
    def parseConfig(self):
        self.config = ConfigParser.ConfigParser()
        self.config.read(self.conffile)
        self.brokerUrl = self.config.get('vhscore', 'BROKERURL', '127.0.0.1')
        self.vhsCoreID = self.config.get('vhscore', 'VHSCOREID', 'vhs-core-0')
        self.vhsDptCoreID = self.config.get('dptcore', 'DPTCOREID', 'vhs-dptcore-0')
        self.vhsDptXdpdID = self.config.get('xdpd', 'XDPDID', 'vhs-xdpd-0')
        for devname in self.config.get('vhscore', 'WANLINKS', '').split():
            self.linkNames['wan'].append(devname)
        for devname in self.config.get('vhscore', 'DMZLINKS', '').split():
            self.linkNames['dmz'].append(devname)
        for devname in self.config.get('vhscore', 'LANLINKS', '').split():
            self.linkNames['lan'].append(devname)
    
    
    def initVhsXdpd(self):
        """   """
        try:
            self.xdpdVhsProxy = proact.common.xdpdproxy.XdpdProxy(self.brokerUrl, self.vhsDptXdpdID)
        except proact.common.xdpdproxy.XdpdProxyException, e:
            print 'VhsCore::initVhsXdpd() initializing xdpd failed ' +  str(e)
            #self.addEvent(proact.common.basecore.BaseCoreEvent(self, self.EVENT_QUIT))

            
        
    def initVhsDptCore(self):
        """   """
        try:
            self.dptCoreProxy = proact.common.dptcore.DptCoreProxy(self.brokerUrl, self.vhsDptCoreID)
            self.dptCoreProxy.reset()
        except proact.common.dptcore.DptCoreProxyException, e:
            print 'VhsCore::initVhsDptCore() initializing dptcore failed ' + str(e)
            #self.addEvent(proact.common.basecore.BaseCoreEvent(self, self.EVENT_QUIT))
    

if __name__ == "__main__":
    VhsCore(wanLinks=['dummy0'], lanLinks=['veth0'], dmzLinks=['veth2']).run() 
    

    