#!/usr/bin/python

from pyroute2.netlink.iproute import *
from pyroute2 import IPRoute
import dns
from qmf.console import Session
import subprocess
import select
import sys
import os
import re
import radvd
import routerlink

EVENT_QUIT = 0
EVENT_IPROUTE = 1
EVENT_RA_ATTACHED = 2
EVENT_RA_DETACHED = 3
EVENT_PREFIX_ATTACHED = 4
EVENT_PREFIX_DETACHED = 5
#CB_IPR = 'CB_IPR'
#CB_EVENT = 'CB_EVENT'




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

    
    def __init__(self, ifaces = {'wan': ['ge0', 'dummy0', 'veth0'], 'lan': ['veth2']}):
       # self.cb_msg_queue = { CB_IPR: [] }
        self.ipr = IPRoute() 
        self.ipr.register_callback(ipr_callback, lambda x: True, [self, ])
        (self.pipein, self.pipeout) = os.pipe()
        self.state = self.STATE_DISCONNECTED
        self.wanLinks = []
        self.lanLinks = []
        self.events = []
        
        for ifname in ifaces['wan']:
            try:
                i = self.ipr.link_lookup(ifname=ifname)[0]
                self.wanLinks.append(routerlink.RouterWanLink(self, ifname, i))
            except:
                print "<error> WAN interface " + ifname + " not found"

        if len(ifaces['lan']) != 1:
            print "invalid number of LAN interfaces, must be 1"
            assert(False)

        for ifname in ifaces['lan']:
            try:
                i = self.ipr.link_lookup(ifname=ifname)[0]
                self.lanLinks.append(routerlink.RouterLanLink(self, ifname, i))
            except:
                print "<error> LAN interface " + ifname + " not found"

        
        # precise starting point: bring all interfaces down
        #
        
        # disable all WAN interfaces
        self.set_interfaces(self.wanLinks, "down")
        # disable all LAN interfaces
        self.set_interfaces(self.lanLinks, "down")
        
        # flush all addresses on WAN interfaces
        self.flush_addresses(self.wanLinks)
        # flush all addresses on LAN interfaces
        self.flush_addresses(self.lanLinks)
        
        
        # bring up WAN interfaces and acquire IPv6 address via RA 
        #
        self.set_interfaces(self.wanLinks, "up")

        self.set_interfaces(self.lanLinks, "up")        
                     

    def __cleanup(self):

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

        # flush all addresses on WAN interfaces
        self.flush_addresses(self.wanLinks)
        # flush all addresses on LAN interfaces
        self.flush_addresses(self.lanLinks)                        
    

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
            
                
    def set_interfaces(self, links, state="up"):
        for link in links:
            try:
                dev = self.ipr.link_lookup(ifname=link.devname)[0]
                #print "link: " + link.devname + " dev: " + str(dev)
                if state == "up":
                    self.ipr.link('set', index=dev, state='up')
                    # net.ipv6.conf.dummy0.accept_ra = 2
                    sysctl_cmd = "sysctl -q -w net.ipv6.conf." + link.devname + ".accept_ra=2"
                    #print sysctl_cmd
                    subprocess.call(sysctl_cmd.split())
                elif state == "down":
                    self.ipr.link('set', index=dev, state='down')
                    # net.ipv6.conf.dummy0.accept_ra = 2
                    sysctl_cmd = "sysctl -q -w net.ipv6.conf." + link.devname + ".accept_ra=0"
                    #print sysctl_cmd
                    subprocess.call(sysctl_cmd.split())

            except IndexError:
                print '<error> set_interfaces ' + link.devname
            
        
    def flush_addresses(self, links):
        for link in links:
            s = "/sbin/ip -6 addr flush dev " + link.devname
            subprocess.call(s.split())
            
    def add_address(self, link, address, prefixlen):
        if not isinstance(link, routerlink.Link):
            print "not of type Link, ignoring"
            return
        s = "/sbin/ip -6 addr add " + address + "/" + str(prefixlen) + " dev " + link.devname
        print s.split()
        subprocess.call(s.split())

    def del_address(self, link, address, prefixlen):
        if not isinstance(link, routerlink.Link):
            print "not of type Link, ignoring"
            return
        s = "/sbin/ip -6 addr del " + address + "/" + str(prefixlen) + " dev " + link.devname
        print s.split()
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
        pass   

    def __handle_new_addr(self, ndmsg):
        # must be an IPv6 address
        if ndmsg['family'] != 10:
            return
        # check all WAN interfaces
        for link in self.wanLinks:
            if link.ifindex == ndmsg['index']:
                try:
                    link.add_addr(ndmsg['attrs'][0][1], ndmsg['prefixlen'])
                except:
                    pass
        # check all LAN interfaces
        for link in self.lanLinks:
            if link.ifindex == ndmsg['index']:
                try:
                    link.add_addr(ndmsg['attrs'][0][1], ndmsg['prefixlen'])
                except:
                    pass


    def __handle_del_route(self, ndmsg):
        pass   

    def __handle_del_link(self, ndmsg):
        pass   

    def __handle_del_addr(self, ndmsg):
        # ignore non IPv6
        if ndmsg['family'] != 10:
            return
        # check all WAN interfaces
        for link in self.wanLinks:
            if link.ifindex == ndmsg['index']:
                try:
                    link.del_addr(ndmsg['attrs'][0][1], ndmsg['prefixlen'])
                except:
                    pass
        # check all LAN interfaces
        for link in self.lanLinks:
            if link.ifindex == ndmsg['index']:
                try:
                    link.del_addr(ndmsg['attrs'][0][1], ndmsg['prefixlen'])
                except:
                    pass


    def __handle_event_ra_attached(self, link):
        print "event RA-ATTACHED: " + str(link)
        if isinstance(link, routerlink.RouterWanLink):
            link.dhcpGetPrefix()
        if isinstance(link, routerlink.RouterLanLink):
            link.startRAs()
        

    def __handle_event_ra_detached(self, link):
        print "event RA-DETACHED: " + str(link)
        if not isinstance(link, routerlink.Link):
            return
        if isinstance(link, routerlink.RouterLanLink):
            link.stopRAs()
        

    def __handle_event_prefix_attached(self, dhclient):
        print "event PREFIX-ATTACHED"
        for lanLink in self.lanLinks:
            lanLink.radvd.add_prefix(radvd.IPv6Prefix(dhclient.new_prefix, 64))
            self.add_address(lanLink, dhclient.new_prefix+'1', 64)

    
    
    def __handle_event_prefix_detached(self, dhclient):
        print "event PREFIX-DETACHED"
        for lanLink in self.lanLinks:
            if lanLink.devname != dhclient.devname:
                continue
            self.del_address(lanLink, dhclient.new_prefix+'1', 64)
            break






if __name__ == "__main__":
    HomeGateway().run()

    


