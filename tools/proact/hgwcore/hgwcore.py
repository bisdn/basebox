#!/usr/bin/python

import sys
sys.path.append('../..')

import proact.common.basecore
import proact.common.radvdaemon
import proact.common.dhcpclient
import ConfigParser
import logging
import time


class HgwCore(proact.common.basecore.BaseCore):
    """
    a description would be useful here
    """
    systemctl_binary = '/usr/bin/systemctl'
    
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
                
            self.conffile = kwargs.get('conffile', 'hgwcore.conf')
            self.parseConfig()
            self.vendor = kwargs.get('vendor', 'bisdn.de')
            self.product = kwargs.get('product', 'hgwcore')
                    
            logging.basicConfig(filename=self.logfile,level=self.loglevel)
                    
            proact.common.basecore.BaseCore.__init__(self, self.brokerUrl, vendor=self.vendor, product=self.product)
            self.logger = logging.getLogger('proact')
            
        except Exception, e:
            self.logger.error('HgwCore::init() failed ' + str(e))
            self.cleanUp()
        
    def cleanUp(self):
        self.logger.info('cleaning up ...')
        try:
            self.agentHandler.cancel()
            self.stopOpenVpn()
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
            link = event.source
            if link.devname in self.linkNames['dmz']:
                link.linkType = 'dmz'
                link.disable()
                link.enable(0)
                self.dmzLinks[link.devname] = link
                self.logger.info('DMZ link found: ' + str(link))
            elif link.devname in self.linkNames['lan']:
                link.linkType = 'lan'
                link.disable()
                link.enable(0)
                self.lanLinks[link.devname] = link
                self.logger.info('LAN link found: ' + str(link))
            elif link.devname in self.linkNames['wan']:
                link.linkType = 'wan'
                link.disable()
                link.enable(2)                
                self.wanLinks[link.devname] = link
                self.logger.info('WAN link found: ' + str(link))
                
        elif event.type == proact.common.basecore.BaseCore.EVENT_DEL_LINK:
            link = event.source
            if link.devname in self.dmzLinks:
                self.dmzLinks.pop(link.devname, None)
                self.logger.info('DMZ link lost: ' + str(link))
            if link.devname in self.lanLinks:
                self.lanLinks.pop(link.devname, None)
                self.logger.info('LAN link lost: ' + str(link))
            if link.devname in self.wanLinks:
                self.wanLinks.pop(link.devname, None)
                self.logger.info('WAN link lost: ' + str(link))
            
        elif event.type == proact.common.basecore.BaseCore.EVENT_NEW_ADDR:
            self.logger.debug('NewAddr (' + str(event.source) + ')')
        elif event.type == proact.common.basecore.BaseCore.EVENT_DEL_ADDR:
            self.logger.debug('DelAddr (' + str(event.source) + ')')
        elif event.type == proact.common.basecore.BaseCore.EVENT_NEW_ROUTE:
            self.logger.debug('NewRoute (' + str(event.source) + ')')
        elif event.type == proact.common.basecore.BaseCore.EVENT_DEL_ROUTE:
            self.logger.debug('DelRoute (' + str(event.source) + ')')
        elif event.type == proact.common.basecore.BaseCore.EVENT_LINK_UP:
            self.logger.debug('LinkUp (' + str(event.source) + ')')
        elif event.type == proact.common.basecore.BaseCore.EVENT_LINK_DOWN:
            self.logger.debug('LinkDown (' + str(event.source) + ')')
        elif event.type == proact.common.basecore.BaseCore.EVENT_RA_ATTACHED:
            self.logger.debug('RA-Attached (' + str(event.source) + ')')
            link = event.source
            if link.devname in self.wanLinks:
                link.dhclient.sendRequest()
            
        elif event.type == proact.common.basecore.BaseCore.EVENT_RA_DETACHED:
            self.logger.debug('RA-Detached (' + str(event.source) + ')')
            link = event.source
            if link.devname in self.wanLinks:
                link.dhclient.killClient()            
            
        elif event.type == proact.common.basecore.BaseCore.EVENT_PREFIX_ATTACHED:
            self.logger.debug('Prefix-Attached (' + str(event.source) + ')')
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
            self.logger.debug('Prefix-Detached (' + str(event.source) + ')')
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
            self.logger.info('RA advertisement started: ' + str(event.source))
        elif event.type == proact.common.basecore.BaseCore.EVENT_RADVD_STOP:
            self.logger.info('RA advertisement stopped: ' + str(event.source))
                                    
    def parseConfig(self):
        self.config = ConfigParser.ConfigParser()
        self.config.read(self.conffile)
        self.brokerUrl = self.config.get('hgwcore', 'BROKERURL', '127.0.0.1')
        self.hgwCoreID = self.config.get('hgwcore', 'HGWCOREID', 'hgw-core-0')
        self.logfile = self.config.get('hgwcore', 'LOGFILE', 'hgwcore.log')
        self.loglevel = self.config.get('hgwcore', 'LOGLEVEL', 'info')
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
        
        for devname in self.config.get('hgwcore', 'WANLINKS', '').split():
            self.linkNames['wan'].append(devname)
        for devname in self.config.get('hgwcore', 'DMZLINKS', '').split():
            self.linkNames['dmz'].append(devname)
        for devname in self.config.get('hgwcore', 'LANLINKS', '').split():
            self.linkNames['lan'].append(devname)

    
    
    
    def startOpenVpn(self):
        """starts the openvpn process connecting this HGW instance with its cloud extension"""
        return
        try:
            openvpn_cmd = self.systemctl_binary + ' start openvpn@home'
            self.logger.debug('executing: ' + str(openvpn_cmd))
            self.process = subprocess.Popen(openvpn_cmd.split(), shell=False, stdout=subprocess.PIPE)
            resultstring = self.process.communicate()
        except:
            self.logger.error('HgwCore::startOpenVpn failed')


    def stopOpenVpn(self):
        """stops the openvpn process connecting this HGW instance with its cloud extension"""
        return
        try:
            openvpn_cmd = self.systemctl_binary + ' stop openvpn@home'
            self.logger.debug('executing: ' + str(openvpn_cmd))
            self.process = subprocess.Popen(openvpn_cmd.split(), shell=False, stdout=subprocess.PIPE)
            resultstring = self.process.communicate()
        except:
            self.logger.error('HgwCore::stopOpenVpn failed')


if __name__ == "__main__":
    HgwCore(wanLinks=['dummy0'], lanLinks=['veth0'], dmzLinks=['veth2']).run() 
    
