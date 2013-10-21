#!/usr/bin/python

from pyroute2.netlink.iproute import *
from pyroute2 import IPRoute
from pyroute2 import IPDB
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
import qmfbroker
import time
import qmf2
import cqmf2
import cqpid
import tunnel
    


EVENT_QUIT = 0
EVENT_IPROUTE = 1
EVENT_RA_ATTACHED = 2
EVENT_RA_DETACHED = 3
EVENT_PREFIX_ATTACHED = 4
EVENT_PREFIX_DETACHED = 5

ifaces = {'wan': ['ge0'], 'lan': ['ge1'], 'dmz': ['veth0']}
brokerUrl = "amqp://127.0.0.1:5672"



class HomeGatewayQmfAgentHandler(qmf2.AgentHandler):
    """
    QMF agent handler for class HomeGateway
    """
    def __init__(self, homegw, agentSession):
        qmf2.AgentHandler.__init__(self, agentSession)
        self.agentSession = agentSession
        
        self.qmfSchemaHomeGateway = qmf2.Schema(qmf2.SCHEMA_TYPE_DATA, "de.bisdn.proact.homegw", "homegw")
        #self.qmfSchemaHomeGateway.addProperty(qmf2.SchemaProperty("dptcoreid", qmf2.SCHEMA_DATA_INT))
        
        self.qmfSchemaHomeGateway_testMethod = qmf2.SchemaMethod("test", desc='testing method for QMF agent support in python')
        self.qmfSchemaHomeGateway_testMethod.addArgument(qmf2.SchemaProperty("subcmd", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaHomeGateway_testMethod.addArgument(qmf2.SchemaProperty("outstring", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_OUT))
        self.qmfSchemaHomeGateway.addMethod(self.qmfSchemaHomeGateway_testMethod)

        # add further methods here ...

        self.agentSession.registerSchema(self.qmfSchemaHomeGateway)

        self.homegw = homegw
        self.qmfDataHomeGateway = qmf2.Data(self.qmfSchemaHomeGateway)
        self.qmfAddrHomeGateway = self.agentSession.addData(self.qmfDataHomeGateway, 'homegw')
        
        
    def method(self, handle, methodName, args, subtypes, addr, userId):
        try:
            if methodName == "test":
                handle.addReturnArgument("outstring", "an output string ...")
                self.agentSession.methodSuccess(handle)
        except:
            self.agentSession.raiseException(handle, "something failed, but we do not know, what ...")
  





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
        (self.pipein, self.pipeout) = os.pipe()
        self.state = self.STATE_DISCONNECTED
        self.wanLinks = []
        self.lanLinks = []
        self.dmzLinks = []
        self.wanDevnames = []
        self.lanDevnames = []
        self.dmzDevnames = []
        self.events = []
        self.tunnels = []

        try:
            self.qmfConnection = cqpid.Connection("127.0.0.1")
            self.qmfConnection.open()
            self.qmfAgentSession = qmf2.AgentSession(self.qmfConnection, "{interval:10, reconnect:True}")
            self.qmfAgentSession.setVendor('bisdn.de')
            self.qmfAgentSession.setProduct('homegw')
            self.qmfAgentSession.open()
            self.qmfAgentHandler = HomeGatewayQmfAgentHandler(self, self.qmfAgentSession)
        except:
            raise

        self.ipr = IPRoute() 
        self.ipr.register_callback(ipr_callback, lambda x: True, [self, ])
        self.ipdb = IPDB(self.ipr)
        
        # get a default brokerURL or the one specified
        #
        brokerUrl = "amqp://127.0.0.1:5672"
        if 'brokerUrl' in kwargs:   
            brokerUrl = kwargs['brokerUrl'] 
                 


        # get access to qmfbroker (xdpd) instance on bottom half of HGW
        #   
        self.qmfbroker = qmfbroker.QmfBroker("xdpid=123, currently ignored", brokerUrl)
        
        self.qmfbroker.lsiDestroy(1000)
        self.qmfbroker.lsiDestroy(2000)
        
        # create LSI for providing routing functionality
        #    
        self.qmfbroker.lsiCreate(1000, "dp0", 3, 4, 2, "172.16.250.65", 6633, 5)

        # create LSI for internal switching functionality
        #
        self.qmfbroker.lsiCreate(2000, "dp1", 3, 4, 2, "172.16.250.65", 6644, 5)

        time.sleep(2)
        
        # create virtual link between both LSI instances
        #
        # this is also auto-attaching the interfaces
        (devname1, devname2) = self.qmfbroker.lsiCreateVirtualLink(dpid1=1000, dpid2=2000)

        # create veth pair via dptcore on data path
        #
        self.qmfbroker.vethLinkCreate('veth0', 'veth1')
        self.dmzDevnames.append('veth0')
        
        time.sleep(2)
       



                
        # extract names for WAN, LAN, DMZ interfaces
        #                 
        if 'ifaces' in kwargs:
            if 'wan' in kwargs['ifaces']: 
                self.wanDevnames = kwargs['ifaces']['wan']    
            if 'dmz' in kwargs['ifaces']: 
                self.dmzDevnames = kwargs['ifaces']['dmz']
        self.lanDevnames = [devname1]




        # attach all WAN, LAN, DMZ ports to routing LSI 
        #
        for devname in self.wanDevnames:
            self.qmfbroker.portAttach(1000, devname)
        for devname in self.dmzDevnames:
            self.qmfbroker.portAttach(1000, devname)
        # PLEASE NOTE:
        # the virtual link LAN interface was already attached during creation to the routing LSI
        
        
        
        # attach ports to switching LSI
        #
        if 'ifaces' in kwargs:
            if 'lan' in kwargs['ifaces']:
                for devname in kwargs['ifaces']['lan']: 
                    self.qmfbroker.portAttach(2000, devname)
        # PLEASE NOTE:
        # the virtual link LAN interface was already attached during creation to the switching LSI




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
  
                     

    def __cleanup(self):
        
        # destroy veth pair via dptcore on data path
        #
        self.qmfbroker.vethLinkDestroy('veth0')

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
            
        # remove data path elements
        # 
        self.qmfbroker.lsiDestroy(1000)
        self.qmfbroker.lsiDestroy(2000)  
        
        # shutdown QMF agent session
        self.qmfAgentSession.close()
        self.qmfConnection.close()
        

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
        if ifname in self.wanDevnames:
            print "NEWLINK (WAN) => " + ifname
            i = self.ipr.link_lookup(ifname=ifname)[0]
            wanLink = routerlink.RouterWanLink(self, ifname, i)
            self.wanLinks.append(wanLink)
            self.wanDevnames.remove(ifname)
            self.set_interfaces([wanLink], "down", 0)
            self.flush_addresses([wanLink])
            self.set_interfaces([wanLink], "up", 2)
        if ifname in self.lanDevnames:
            print "NEWLINK (LAN) => " + ifname
            i = self.ipr.link_lookup(ifname=ifname)[0]
            lanLink = routerlink.RouterLanLink(self, ifname, i)
            self.lanLinks.append(lanLink)
            self.lanDevnames.remove(ifname)
            self.set_interfaces([lanLink], "down", 0)
            self.flush_addresses([lanLink])
            self.set_interfaces([lanLink], "up", 0)
        if ifname in self.dmzDevnames:
            print "NEWLINK (DMZ) => " + ifname
            i = self.ipr.link_lookup(ifname=ifname)[0]
            dmzLink = routerlink.RouterDmzLink(self, ifname, i)
            self.dmzLinks.append()
            self.dmzDevnames.remove(ifname)
            self.set_interfaces([dmzLink], "down", 0)
            self.flush_addresses([dmzLink])
            self.set_interfaces([dmzLink], "up", 0)
                
                        
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
                
        # for l2tp tunnel
        for dmzLink in self.dmzLinks:
            if dmzLink.devname != 'veth0':
                continue
            for prefix in dhclient.new_prefixes:
                p = prefix.get_subprefix(dmzLink.ifindex).prefix
                cpeIP = str(p)+'2'
                defrouter = str(p)+'1'
                self.qmfbroker.linkAddIP('veth1', cpeIP, 64, defrouter)
                for tun in self.tunnels:
                    if tun.cpeIP == cpeIP:
                        break
                else:
                    # TODO: get a unique tunnel ID
                    cpeTunnelID = 10
                    # TODO: get a unique session ID
                    cpeSessionID = 1
                    cpeUdpPort = 6000
                    cpeDevname = 'l2tpeth' + str(cpeTunnelID)
                    (vhsTunnelID, vhsSessionID, vhsIP, vhsUdpPort) = self.qmfbroker.vhsAttach('l2tp', cpeTunnelID, cpeSessionID, cpeIP, cpeUdpPort)
                    tun = tunnel.Tunnel(cpeDevname, cpeTunnelID, vhsTunnelID, cpeSessionID, vhsSessionID, cpeIP, vhsIP, cpeUdpPort, vhsUdpPort)
                    self.tunnels.append(tun)
                    print "CREATING TUNNEL => " + str(tun)
                    self.qmfbroker.l2tpCreateTunnel(tun.cpeTunnelID, 
                                                    tun.vhsTunnelID,
                                                    tun.vhsIP,
                                                    tun.cpeIP,
                                                    tun.cpeUdpPort,
                                                    tun.vhsUdpPort)
                    self.qmfbroker.l2tpCreateSession(tun.cpeDevname,
                                                     tun.cpeTunnelID,
                                                     tun.cpeSessionID,
                                                     tun.vhsSessionID) 
                                                     
        print "[E] event PREFIX-ATTACHED"
    
    
    def __handle_event_prefix_detached(self, dhclient):
        print "[S] event PREFIX-DETACHED"

        # for l2tp tunnel
        for dmzLink in self.dmzLinks:
            if dmzLink.devname != 'veth0':
                continue    
            for prefix in dhclient.new_prefixes:
                p = prefix.get_subprefix(dmzLink.ifindex).prefix
                cpeIP = str(p)+'2'
                self.qmfbroker.linkDelIP('veth1', cpeIP, 64)
                for tun in self.tunnels:
                    if tun.cpeIP == cpeIP:
                        print "DESTROYING TUNNEL => " + str(tun)
                        self.qmfbroker.vhsDetach('l2tp', tun.cpeTunnelID, tun.cpeSessionID)
                        self.qmfbroker.l2tpDestroySession(tun.cpeDevname, tun.cpeTunnelID, tun.cpeSessionID)
                        self.qmfbroker.l2tpDestroyTunnel(tun.cpeTunnelID)
                        self.tunnels.remove(tun)
                        break
                        
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
    HomeGateway(ifaces=ifaces, brokerUrl=brokerUrl).run()

    


