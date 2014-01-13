#!/usr/bin/python

import sys
sys.path.append('../..')

import proact.common.basecore
import ConfigParser
import logging
import time



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
            
            for devname in kwargs.get('wanLinks', []):
                self.linkNames['wan'].append(devname)
            for devname in kwargs.get('dmzLinks', []):
                self.linkNames['dmz'].append(devname)
            for devname in kwargs.get('lanLinks', []):
                self.linkNames['lan'].append(devname)

            self.conffile = kwargs.get('conffile', 'vhscore.conf')
            self.parseConfig()
            self.brokerUrl = ""
            self.vendor = kwargs.get('vendor', 'bisdn.de')
            self.product = kwargs.get('product', 'vhscore')
            
            logging.basicConfig(filename=self.logfile,level=self.loglevel)
                    
            proact.common.basecore.BaseCore.__init__(self, self.brokerUrl, vendor=self.vendor, product=self.product)
            self.logger = logging.getLogger('proact')
            
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
        logging.info('reading config file ' + self.conffile)
        self.config = ConfigParser.ConfigParser()
        self.config.read(self.conffile)
        self.logfile = self.config.get('vhscore', 'LOGFILE', 'hgwcore.log')
        self.loglevel = self.config.get('vhscore', 'LOGLEVEL', 'info')
        if self.loglevel.lower() == 'debug':
            self.loglevel = logging.DEBUG
        elif self.loglevel.lower() == 'info':
            self.loglevel = logging.INFO
        elif self.loglevel.lower() == 'warning':
            self.loglevel = logging.WARNING
        elif self.loglevel.lower() == 'error':
            self.loglevel = logging.ERROR
        elif self.loglevel.lower() == 'critical':
            self.loglevel = logging.CRITICAL
        else:
            self.loglevel = logging.INFO
        
        for devname in self.config.get('vhscore', 'WANLINKS', '').split():
            self.linkNames['wan'].append(devname)
        for devname in self.config.get('vhscore', 'DMZLINKS', '').split():
            self.linkNames['dmz'].append(devname)
        for devname in self.config.get('vhscore', 'LANLINKS', '').split():
            self.linkNames['lan'].append(devname)
    
 

if __name__ == "__main__":
    VhsCore(wanLinks=['dummy0'], lanLinks=['veth0'], dmzLinks=['veth2']).run() 
    

    