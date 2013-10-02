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

CB_IPR = 'CB_IPR'
RA_DETACHED = 0
RA_ATTACHED = 1


def ipr_callback(msg, obj):
    #print msg['event']
    obj.notify(CB_IPR, msg)


class HomeGateway(object):
    """Main class for ProAct Home Gateway"""
    STATE_DISCONNECTED = "DISCONNECTED"
    STATE_CONNECTED = "CONNECTED"

    
    def __init__(self, ifaces = {'wan': ['ge0', 'dummy0', 'veth0'], 'lan': ['ge1', 'veth2']}):
        self.cb_msg_queue = { CB_IPR: [] }
        self.ipr = IPRoute() 
        self.ipr.register_callback(ipr_callback, lambda x: True, [self, ])
        (self.pipein, self.pipeout) = os.pipe()
        self.state = self.STATE_DISCONNECTED
        self.wanLinks = []
        self.lanLinks = []
        
        for ifname in ifaces['wan']:
            try:
                i = self.ipr.link_lookup(ifname=ifname)[0]
                self.wanLinks.append(RouterWanLink(ifname, i, "down", RA_DETACHED))
            except:
                print "<error> WAN interface " + ifname + " not found"

        for ifname in ifaces['lan']:
            try:
                i = self.ipr.link_lookup(ifname=ifname)[0]
                self.lanLinks.append(RouterLanLink(ifname, i, "down", RA_DETACHED))
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
                     
            
    def notify(self, cb_type, msg):
        if cb_type == CB_IPR:
            self.cb_msg_queue[CB_IPR].append(msg)
        os.write(self.pipeout, cb_type)
    
    
    
    def run(self):
        self.ep = select.epoll()
        self.ep.register(self.pipein, select.EPOLLET | select.EPOLLIN)
        
        while True:
            if self.state == self.STATE_DISCONNECTED:
                pass
            elif self.state == self.STATE_CONNECTED:
                pass
            self.ep.poll(timeout=-1, maxevents=1)
            msg = os.read(self.pipein, 256)
            #print "epoll done: " + msg + "\n"
            if msg == CB_IPR:
                for ndmsg in self.cb_msg_queue[CB_IPR]:
                    self.__handle_ndmsg(ndmsg)
                self.cb_msg_queue[CB_IPR] = []
                
                
    def set_interfaces(self, links, state="up"):
        for link in links:
            try:
                dev = self.ipr.link_lookup(ifname=link.devname)[0]
                print "link: " + link.devname + " dev: " + str(dev)
                if state == "up":
                    self.ipr.link('set', index=dev, state='up')
                    # net.ipv6.conf.dummy0.accept_ra = 2
                    sysctl_cmd = "sysctl -w net.ipv6.conf." + link.devname + ".accept_ra=2"
                    print sysctl_cmd
                    subprocess.call(sysctl_cmd.split())
                elif state == "down":
                    self.ipr.link('set', index=dev, state='down')
                    # net.ipv6.conf.dummy0.accept_ra = 2
                    sysctl_cmd = "sysctl -w net.ipv6.conf." + link.devname + ".accept_ra=0"
                    print sysctl_cmd
                    subprocess.call(sysctl_cmd.split())

            except IndexError:
                print '<error> set_interfaces ' + link.devname
            
            
    def flush_addresses(self, links):
        for link in links:
            s = "/sbin/ip -6 addr flush dev " + link.devname
            subprocess.call(s.split())

    def __handle_ndmsg(self, ndmsg):
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




class Link(object):
    """abstraction for a link"""
    def __init__(self, devname, ifindex, state, attached = RA_DETACHED):
        self.devname = devname
        self.ifindex = ifindex
        self.state = state
        self.attached = attached
        self.addresses = {}
        self.linklocal = {}
        
    def __str__(self):
        laddr = '('
        for addr in self.linklocal:
            laddr += addr + ', '
        laddr += ')'
        saddr = '('
        for addr in self.addresses:
            saddr += addr + ', '
        saddr += ')'
        return 'Link(dev='+self.devname+', ifindex='+str(self.ifindex)+', attached='+str(self.attached)+', '+saddr+', '+laddr+')'
    
    def add_addr(self, addr, prefixlen):
        if not re.match('fe80', addr):
            if not addr in self.addresses:
                self.addresses[addr] = { 'prefixlen': prefixlen }
                self.attached = RA_ATTACHED
                print str(self)
        else:
            if not addr in self.linklocal:
                self.linklocal[addr] = { 'prefixlen': prefixlen }
                print str(self)

    def del_addr(self, addr, prefixlen):
        if not re.match('fe80', addr):
            if addr in self.addresses:
                del self.addresses[addr]
                print str(self)
            if not self.addresses:
                self.attached = RA_DETACHED
                print str(self)
        else:        
            if addr in self.linklocal:
                self.linklocal[addr]
                print str(self)
        
        
        
class RouterWanLink(Link):
    """abstraction for an uplink on the HG's router component"""
    def __init__(self, devname, ifindex, state, attached):
        super(RouterWanLink, self).__init__(devname, ifindex, state, attached)

    def __str__(self):
        return 'RouterWanLink('+super(RouterWanLink, self).__str__()+')' 




class RouterLanLink(Link):
    """abstraction for a downlink on the HG's router component"""
    def __init__(self, devname, ifindex, state, attached):
        super(RouterLanLink, self).__init__(devname, ifindex, state, attached)

    def __str__(self):
        return 'RouterLanLink('+super(RouterLanLink, self).__str__()+')' 



class Dhclient(object):
    """class controls a single dhclient instance"""
    def __init__(self):
        pass
    
    





if __name__ == "__main__":
    HomeGateway().run()



