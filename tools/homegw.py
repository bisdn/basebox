#!/usr/bin/python

from pyroute2.netlink.iproute import *
from pyroute2 import IPRoute
import dns
from qmf.console import Session
import subprocess
import select
import sys
import os

CB_IPR = 'CB_IPR'

def ipr_callback(msg, obj):
    #print msg['event']
    obj.notify(CB_IPR, msg)


class HomeGateway:
    """Main class for ProAct Home Gateway"""
    STATE_DISCONNECTED = "DISCONNECTED"
    STATE_CONNECTED = "CONNECTED"
    
    def __init__(self, ifaces = {'wan': ['ge0', 'dummy0', 'veth0'], 'lan': ['ge1']}):
        self.cb_msg_queue = { CB_IPR: [] }
        self.ipr = IPRoute() 
        self.ipr.register_callback(ipr_callback, lambda x: True, [self, ])
        (self.pipein, self.pipeout) = os.pipe()
        self.wan_ifaces = ifaces['wan']
        self.lan_ifaces = ifaces['lan']
        self.state = self.STATE_DISCONNECTED
        
        # precise starting point: bring all interfaces down
        #
        
        # disable all WAN interfaces
        self.set_interfaces(self.wan_ifaces, "down")
        # disable all LAN interfaces
        self.set_interfaces(self.lan_ifaces, "down")
        
        # flush all addresses on WAN interfaces
        self.flush_addresses(self.wan_ifaces)
        # flush all addresses on LAN interfaces
        self.flush_addresses(self.lan_ifaces)
        
        
        # bring up WAN interfaces and acquire IPv6 address via RA 
        #
        self.set_interfaces(self.wan_ifaces, "up")
        
                     
            
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
                
                
    def set_interfaces(self, ifaces, state="up"):
        for iface in ifaces:
            try:
                dev = self.ipr.link_lookup(ifname=iface)[0]
                print "iface: " + iface + " dev: " + str(dev)
                if state == "up":
                    self.ipr.link('set', index=dev, state='up')
                    # net.ipv6.conf.dummy0.accept_ra = 2
                    sysctl_cmd = "sysctl -w net.ipv6.conf." + iface + ".accept_ra=2"
                    subprocess.call(sysctl_cmd.split())
                elif state == "down":
                    self.ipr.link('set', index=dev, state='down')
                    # net.ipv6.conf.dummy0.accept_ra = 2
                    sysctl_cmd = "sysctl -w net.ipv6.conf." + iface + ".accept_ra=0"
                    subprocess.call(sysctl_cmd.split())

            except IndexError:
                pass
            
            
    def flush_addresses(self, ifaces):
        for iface in ifaces:
            s = "/sbin/ip -6 addr flush dev " + iface
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
        print "NEW-ADDR index: " + str(ndmsg['index']) 
        print ndmsg 

    def __handle_del_route(self, ndmsg):
        pass   

    def __handle_del_link(self, ndmsg):
        pass   

    def __handle_del_addr(self, ndmsg):
        pass   






if __name__ == "__main__":
    HomeGateway().run()



