#!/usr/bin/python

from pyroute2.netlink.iproute import *
from pyroute2 import IPRoute
import dns
from qmf.console import Session
import subprocess
import netaddr
import select
import sys
import dns.resolver
import os
import re
import radvd
import routerlink
import datapath

EVENT_QUIT = 0
EVENT_IPROUTE = 1
EVENT_RA_ATTACHED = 2
EVENT_RA_DETACHED = 3
EVENT_PREFIX_ATTACHED = 4
EVENT_PREFIX_DETACHED = 5

my_ifaces = {'wan': ['ge0', 'dummy0', 'veth0'], 'lan': ['ge1', 'veth2'], 'dmz': ['veth4']}
my_brokerUrl = "amqp://172.16.252.21:5672"


class HomeGatewayEvent(object):
    """Base class for all events generated within class HomeGateway"""
    def __init__(self, source, type, opaque=''):
        self.source = source
        self.type = type
        self.opaque = opaque
        #print self
    
    def __str__(self, *args, **kwargs):
        return '<HomeGatewayEvent [type:' + str(self.type) + '] ' + object.__str__(self, *args, **kwargs) + ' >'


def ipr_callback(ndmsg, homeGateway):
    if not isinstance(homeGateway, HomeGateway):
        print "invalid type"
        return
    homeGateway.add_event(HomeGatewayEvent("", EVENT_IPROUTE, ndmsg))


class HomeGateway(object):
    """Main class for ProAct Home Gateway"""
    STATE_DISCONNECTED = "DISCONNECTED"
    STATE_CONNECTED = "CONNECTED"

    
    def __init__(self, **kwargs): 
        self.ipr = IPRoute() 
        self.ipr.register_callback(ipr_callback, lambda x: True, [self, ])
        (self.pipein, self.pipeout) = os.pipe()
        self.state = self.STATE_DISCONNECTED
        self.wanLinks = []
        self.lanLinks = []
        self.dmzLinks = []
        self.wanDevnames = []
        self.lanDevnames = []
        self.dmzDevnames = []
        self.events = []

        brokerUrl = "amqp://127.0.0.1:5672"
        if 'brokerUrl' in kwargs:   
            brokerUrl = kwargs['brokerUrl'] 
            
        self.datapath = datapath.DataPath("xdpid=123, currently ignored", brokerUrl)    
        self.datapath.lsiCreate(1000, "dp0", 3, 8, 2, "172.16.250.65", 6633, 5)
        self.datapath.portAttach(1000, "ge0")
        self.datapath.portAttach(1000, "ge1")
                
        if 'ifaces' in kwargs:
            if 'wan' in kwargs['ifaces']: 
                self.wanDevnames = kwargs['ifaces']['wan']
            if 'lan' in kwargs['ifaces']: 
                self.lanDevnames = kwargs['ifaces']['lan']
            if 'dmz' in kwargs['ifaces']: 
                self.dmzDevnames = kwargs['ifaces']['dmz']

        for ifname in self.wanDevnames[:]:
            try:
                i = self.ipr.link_lookup(ifname=ifname)[0]
                self.wanLinks.append(routerlink.RouterWanLink(self, ifname, i))
                self.wanDevnames.remove(ifname)
            except:
                print "<error> WAN interface " + ifname + " not found"

        for ifname in self.lanDevnames[:]:
            try:
                i = self.ipr.link_lookup(ifname=ifname)[0]
                self.lanLinks.append(routerlink.RouterLanLink(self, ifname, i))
                self.lanDevnames.remove(ifname)
            except:
                print "<error> LAN interface " + ifname + " not found"

        for ifname in self.dmzDevnames[:]:
            try:
                i = self.ipr.link_lookup(ifname=ifname)[0]
                self.dmzLinks.append(routerlink.RouterDmzLink(self, ifname, i))
                self.dmzDevnames.remove(ifname)
            except:
                print "<error> DMZ interface " + ifname + " not found"

        
        # precise starting point: bring all interfaces down
        #
        
        # disable all WAN interfaces
        self.set_interfaces(self.wanLinks, "down", 0)
        # disable all LAN interfaces
        self.set_interfaces(self.lanLinks, "down", 0)
        # disable all DMZ interfaces
        self.set_interfaces(self.dmzLinks, "down", 0)
        
        # flush all addresses on WAN interfaces
        self.flush_addresses(self.wanLinks)
        # flush all addresses on LAN interfaces
        self.flush_addresses(self.lanLinks)
        # flush all addresses on DMZ interfaces
        self.flush_addresses(self.dmzLinks)
        
        
        # bring up WAN interfaces and acquire IPv6 address via RA 
        #
        self.set_interfaces(self.wanLinks, "up", 2)

        self.set_interfaces(self.lanLinks, "up", 0)        
                     
        self.set_interfaces(self.dmzLinks, "up", 0)     
        
        # remove data path
        # 
        self.datapath.lsiDestroy(1000)   
                     

    def __cleanup(self):

        # TODO: stop whatever ...            
        for dmzLink in self.dmzLinks:
            pass

        # stop announcement of obtained public prefixes            
        for lanLink in self.lanLinks:
            lanLink.stopRAs()

        # release addresses
        for wanLink in self.wanLinks:
            wanLink.dhcpReleasePrefix()
        
        # disable all WAN interfaces
        self.set_interfaces(self.wanLinks, "down")
        # disable all LAN interfaces
        self.set_interfaces(self.lanLinks, "down")
        # disable all LAN interfaces
        self.set_interfaces(self.dmzLinks, "down")

        # flush all addresses on WAN interfaces
        self.flush_addresses(self.wanLinks)
        # flush all addresses on LAN interfaces
        self.flush_addresses(self.lanLinks)                        
        # flush all addresses on LAN interfaces
        self.flush_addresses(self.dmzLinks)                        
    

    def add_event(self, event):
        #if not isinstance(event, HomeGatewayEvent):
        #    print "trying to add non-event, ignoring"
        #    print str(event)
        #    return
        self.events.append(event)
        os.write(self.pipeout, str(event.type))

    
    def run(self):
        self.ep = select.epoll()
        self.ep.register(self.pipein, select.EPOLLET | select.EPOLLIN)
        
        
        while True:
            try :
                self.ep.poll(timeout=-1, maxevents=1)
                evType = os.read(self.pipein, 256)
                #print "epoll done: " + evType + "\n"
                
                for event in self.events:
                    if event.type == EVENT_QUIT:
                        return
                    if event.type == EVENT_IPROUTE:
                        self.__handle_event_iproute(event.opaque)
                    if event.type == EVENT_RA_ATTACHED:
                        self.__handle_event_ra_attached(event.opaque)
                    if event.type == EVENT_RA_DETACHED:
                        self.__handle_event_ra_detached(event.opaque)
                    if event.type == EVENT_PREFIX_ATTACHED:
                        self.__handle_event_prefix_attached(event.opaque)
                    if event.type == EVENT_PREFIX_DETACHED:
                        self.__handle_event_prefix_detached(event.opaque)
                else:
                    self.events = []
            except KeyboardInterrupt:
                self.__cleanup()
                self.add_event(HomeGatewayEvent(self, EVENT_QUIT))
            
                
    def set_interfaces(self, links, state="up", accept_ra=2):
        for link in links:
            try:
                dev = self.ipr.link_lookup(ifname=link.devname)[0]
                #print "link: " + link.devname + " dev: " + str(dev)
                if state == "up":
                    self.ipr.link('set', index=dev, state='up')
                    # net.ipv6.conf.dummy0.accept_ra = 2
                    sysctl_cmd = "sysctl -q -w net.ipv6.conf." + link.devname + ".accept_ra=" + str(accept_ra)
                    #print sysctl_cmd
                    subprocess.call(sysctl_cmd.split())
                elif state == "down":
                    self.ipr.link('set', index=dev, state='down')
                    # net.ipv6.conf.dummy0.accept_ra = 2
                    sysctl_cmd = "sysctl -q -w net.ipv6.conf." + link.devname + ".accept_ra=" + str(accept_ra)
                    #print sysctl_cmd
                    subprocess.call(sysctl_cmd.split())

            except IndexError:
                print '<error> set_interfaces ' + link.devname
            
        
    def flush_addresses(self, links):
        for link in links:
            s = "/sbin/ip -6 addr flush dev " + link.devname
            subprocess.call(s.split())
            

    def __handle_event_iproute(self, ndmsg):
        #print ndmsg['event']
        event = ndmsg['event']
        if   event == 'RTM_NEWROUTE':
            self.__handle_new_route(ndmsg)
        elif event == 'RTM_NEWLINK':
            self.__handle_new_link(ndmsg)
        elif event == 'RTM_NEWADDR':
            self.__handle_new_addr(ndmsg)
        elif event == 'RTM_DELROUTE':
            self.__handle_del_route(ndmsg)
        elif event == 'RTM_DELLINK':
            self.__handle_del_link(ndmsg)
        elif event == 'RTM_DELADDR':
            self.__handle_del_addr(ndmsg)
               
               
               
    def __handle_new_route(self, ndmsg):
        pass   

    def __handle_new_link(self, ndmsg):
        # + str(ndmsg)
        ifname = ndmsg['attrs'][0][1]
        print "NEWLINK => " + ifname
        if ifname in self.wanDevnames:
            i = self.ipr.link_lookup(ifname=ifname)[0]
            self.wanLinks.append(routerlink.RouterWanLink(self, ifname, i))
            self.wanDevnames.remove(ifname)
        if ifname in self.lanDevnames:
            i = self.ipr.link_lookup(ifname=ifname)[0]
            self.lanLinks.append(routerlink.RouterLanLink(self, ifname, i))
            self.lanDevnames.remove(ifname)
        if ifname in self.dmzDevnames:
            i = self.ipr.link_lookup(ifname=ifname)[0]
            self.dmzLinks.append(routerlink.RouterDmzLink(self, ifname, i))
            self.dmzDevnames.remove(ifname)
                
                
    def __handle_new_addr(self, ndmsg):
        # must be an IPv6 address
        if ndmsg['family'] != 10:
            return
        # check all WAN interfaces
        for link in self.wanLinks:
            if link.ifindex == ndmsg['index']:
                try:
                    link.nl_add_addr(ndmsg['attrs'][0][1], ndmsg['prefixlen'])
                except:
                    pass
                print str(link)
        # check all LAN interfaces
        for link in self.lanLinks:
            if link.ifindex == ndmsg['index']:
                try:
                    link.nl_add_addr(ndmsg['attrs'][0][1], ndmsg['prefixlen'])
                except:
                    pass
                print str(link)
        # check all DMZ interfaces
        for link in self.dmzLinks:
            if link.ifindex == ndmsg['index']:
                try:
                    link.nl_add_addr(ndmsg['attrs'][0][1], ndmsg['prefixlen'])
                except:
                    pass
                print str(link)


    def __handle_del_route(self, ndmsg):
        pass   


    def __handle_del_link(self, ndmsg):
        # +  str(ndmsg)
        ifname = ndmsg['attrs'][0][1]
        print "DELLINK => " + ifname 
        for wanLink in self.wanLinks:
            if ifname == wanLink.devname:
                self.wanDevnames.append(ifname)
                self.wanLinks.remove(wanLink)
                break
        for lanLink in self.lanLinks:
            if ifname == lanLink.devname:
                self.lanDevnames.append(ifname)
                self.lanLinks.remove(lanLink)
                break
        for dmzLink in self.dmzLinks:
            if ifname == dmzLink.devname:
                self.dmzDevnames.append(ifname)
                self.dmzLinks.remove(dmzLink)
                break



    def __handle_del_addr(self, ndmsg):
        # ignore non IPv6
        if ndmsg['family'] != 10:
            return
        # check all WAN interfaces
        for link in self.wanLinks:
            if link.ifindex == ndmsg['index']:
                try:
                    link.nl_del_addr(ndmsg['attrs'][0][1], ndmsg['prefixlen'])
                except:
                    pass
                print str(link)
        # check all LAN interfaces
        for link in self.lanLinks:
            if link.ifindex == ndmsg['index']:
                try:
                    link.nl_del_addr(ndmsg['attrs'][0][1], ndmsg['prefixlen'])
                except:
                    pass
                print str(link)
        # check all LAN interfaces
        for link in self.dmzLinks:
            if link.ifindex == ndmsg['index']:
                try:
                    link.nl_del_addr(ndmsg['attrs'][0][1], ndmsg['prefixlen'])
                except:
                    pass
                print str(link)


    def __handle_event_ra_attached(self, link):
        print "[S] event RA-ATTACHED: " + str(link)
        if isinstance(link, routerlink.RouterWanLink):
            link.dhcpGetPrefix()
        if isinstance(link, routerlink.RouterLanLink):
            link.startRAs()
        if isinstance(link, routerlink.RouterDmzLink):
            link.startRAs()
        print "[E] event RA-ATTACHED: " + str(link)
        

    def __handle_event_ra_detached(self, link):
        print "[S] event RA-DETACHED: " + str(link)
        if not isinstance(link, routerlink.Link):
            return
        if isinstance(link, routerlink.RouterLanLink):
            link.stopRAs()
        if isinstance(link, routerlink.RouterDmzLink):
            link.stopRAs()
        print "[E] event RA-DETACHED: " + str(link)
        

    def __handle_event_prefix_attached(self, dhclient):
        print "[S] event PREFIX-ATTACHED"
        
        try:
            answers = dns.resolver.query('bisdn.de', 'MX')
            for answer in answers:
                print answer
        except dns.resolver.NoAnswer:
            pass
        
        for lanLink in self.lanLinks:
            for prefix in dhclient.new_prefixes:
                p = prefix.get_subprefix(lanLink.ifindex).prefix
                lanLink.radvd.add_prefix(radvd.IPv6Prefix(str(p), 64))
            for prefix in dhclient.new_prefixes:
                p = prefix.get_subprefix(lanLink.ifindex).prefix
                lanLink.ip_addr_add(str(p)+'1', 64)
                
        for dmzLink in self.dmzLinks:
            for prefix in dhclient.new_prefixes:
                p = prefix.get_subprefix(dmzLink.ifindex).prefix
                dmzLink.radvd.add_prefix(radvd.IPv6Prefix(str(p), 64))
            for prefix in dhclient.new_prefixes:
                p = prefix.get_subprefix(dmzLink.ifindex).prefix
                dmzLink.ip_addr_add(str(p)+'1', 64)
        
        print "[E] event PREFIX-ATTACHED"
    
    
    def __handle_event_prefix_detached(self, dhclient):
        print "[S] event PREFIX-DETACHED"
        
        for lanLink in self.lanLinks:
            for prefix in dhclient.new_prefixes:
                p = prefix.get_subprefix(lanLink.ifindex).prefix
                lanLink.ip_addr_del(str(p)+'1', 64)
        
        for dmzLink in self.dmzLinks:
            for prefix in dhclient.new_prefixes:
                p = prefix.get_subprefix(dmzLink.ifindex).prefix
                dmzLink.ip_addr_del(str(p)+'1', 64)
            
        print "[E] event PREFIX-DETACHED"





if __name__ == "__main__":
    HomeGateway(ifaces=my_ifaces, brokerUrl=my_brokerUrl).run()

    


