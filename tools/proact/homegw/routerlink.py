#!/usr/bin/python

#from homegw import HomeGatewayEvent
import homegw
import dhcpclient
import radvd
import subprocess
import select
import sys
import os
import re

STATE_RA_DETACHED = 0
STATE_RA_ATTACHED = 1

class Link(object):
    """abstraction for a link"""
    def __init__(self, parent, devname, ifindex, uniquePrefix, state = STATE_RA_DETACHED):
        self.parent = parent
        self.devname = devname
        self.uniquePrefix = uniquePrefix
        self.ifindex = ifindex
        self.state = state
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
        return 'Link(dev='+self.devname+', ifindex='+str(self.ifindex)+', uniquePrefix='+str(self.uniquePrefix)+', state='+str(self.state)+', '+saddr+', '+laddr+')'
    
    def nl_add_addr(self, addr, prefixlen):
        if not re.match('fe80', addr):
            if not addr in self.addresses:
                self.addresses[addr] = { 'prefixlen': prefixlen }
                if self.state != STATE_RA_ATTACHED:
                    self.state = STATE_RA_ATTACHED
                    self.parent.add_event(homegw.HomeGatewayEvent(self, homegw.EVENT_RA_ATTACHED, self))
        else:
            if not addr in self.linklocal:
                self.linklocal[addr] = { 'prefixlen': prefixlen }

    def nl_del_addr(self, addr, prefixlen):
        if not re.match('fe80', addr):
            if addr in self.addresses:
                del self.addresses[addr]
            if not self.addresses:
                if self.state != STATE_RA_DETACHED:
                    self.state = STATE_RA_DETACHED
                    self.parent.add_event(homegw.HomeGatewayEvent(self, homegw.EVENT_RA_DETACHED, self))
        else:        
            if addr in self.linklocal:
                self.linklocal[addr]
                
    def ip_addr_flush(self):
        s = "/sbin/ip -6 addr flush dev " + self.devname
        subprocess.call(s.split())
        
    def ip_addr_add(self, address, prefixlen):
        # once the address has been added, netlink will inform us and trigger self.add_addr()
        s = "/sbin/ip -6 addr add " + address + "/" + str(prefixlen) + " dev " + self.devname
        print 'adding address: ' + s
        subprocess.call(s.split())

    def ip_addr_del(self, address, prefixlen):
        s = "/sbin/ip -6 addr del " + address + "/" + str(prefixlen) + " dev " + self.devname
        print 'deleting address: ' + s
        subprocess.call(s.split())

    def is_RA_attached(self):
        if self.state == STATE_RA_ATTACHED:
            return True
        else:
            return False
        
    def is_PREFIX_attached(self):
        return self.dhclient.is_PREFIX_attached()
        
        
        
class RouterWanLink(Link):
    """abstraction for an uplink on the HG's router component"""
    def __init__(self, parent, devname, ifindex, uniquePrefix, state=STATE_RA_DETACHED):
        super(RouterWanLink, self).__init__(parent, devname, ifindex, uniquePrefix, state)
        self.dhclient = dhcpclient.DhcpClient(parent, devname)

    def __str__(self):
        return '<RouterWanLink ' + super(RouterWanLink, self).__str__() + ' ' + str(self.dhclient) + ' >' 
    
    def dhcpGetPrefix(self):
        self.dhclient.request()

    def dhcpReleasePrefix(self):
        self.dhclient.release()
    



class RouterLanLink(Link):
    """abstraction for a downlink on the HG's router component"""
    def __init__(self, parent, devname, ifindex, uniquePrefix, state=STATE_RA_DETACHED):
        super(RouterLanLink, self).__init__(parent, devname, ifindex, uniquePrefix, state)
        self.radvd = radvd.RAdvd(parent, devname)
        print str(self)

    def __str__(self):
        return '<RouterLanLink ' + super(RouterLanLink, self).__str__() + ' ' + str(self.radvd) + ' >' 
    
    def startRAs(self):
        self.radvd.start()

    def stopRAs(self):
        self.radvd.stop()




class RouterDmzLink(Link):
    """abstraction for a downlink on the HG's router component"""
    def __init__(self, parent, devname, ifindex, uniquePrefix, state=STATE_RA_DETACHED):
        super(RouterDmzLink, self).__init__(parent, devname, ifindex, uniquePrefix, state)
        self.radvd = radvd.RAdvd(parent, devname)
        print str(self)

    def __str__(self):
        return '<RouterDmzLink ' + super(RouterDmzLink, self).__str__() + ' >' 

    def startRAs(self):
        self.radvd.start()

    def stopRAs(self):
        self.radvd.stop()
    




if __name__ == "__main__":
    pass

    