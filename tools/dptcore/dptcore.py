#!/usr/bin/python

from pyroute2 import IPDB
import subprocess
import select
import qmf2
import cqmf2
import cqpid
import sys
import os
from l2tp import L2tpSession, L2tpTunnel

EVENT_QUIT = 0


    
class DptCoreQmfAgentHandler(qmf2.AgentHandler):
    def __init__(self, dptcore, agentSession):
        qmf2.AgentHandler.__init__(self, agentSession)
        self.agentSession = agentSession
        self.__set_schema()
        self.dptcore = dptcore
        self.qmf_dptcore = {}
        self.qmf_dptcore['data'] = qmf2.Data(self.sch_dptcore)
        self.qmf_dptcore['addr'] = self.agentSession.addData(self.qmf_dptcore['data'], 'dptcore')

        
    def __set_schema(self):
        self.sch_dptcore = qmf2.Schema(qmf2.SCHEMA_TYPE_DATA, "de.bisdn.dptcore", "dptcore")
        #self.sch_dptcore.addProperty(qmf2.SchemaProperty("dptcoreid", qmf2.SCHEMA_DATA_INT))
        
        self.sch_dptcore_testMethod = qmf2.SchemaMethod("test", desc='testing method for QMF agent support in python')
        self.sch_dptcore_testMethod.addArgument(qmf2.SchemaProperty("subcmd", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_dptcore_testMethod.addArgument(qmf2.SchemaProperty("outstring", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_OUT))
        self.sch_dptcore.addMethod(self.sch_dptcore_testMethod)

        self.sch_l2tpCreateTunnelMethod = qmf2.SchemaMethod("l2tpCreateTunnel", desc='create L2TP tunnel')
        self.sch_l2tpCreateTunnelMethod.addArgument(qmf2.SchemaProperty("tunnel_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.sch_l2tpCreateTunnelMethod.addArgument(qmf2.SchemaProperty("peer_tunnel_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.sch_l2tpCreateTunnelMethod.addArgument(qmf2.SchemaProperty("remote", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_l2tpCreateTunnelMethod.addArgument(qmf2.SchemaProperty("local", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_l2tpCreateTunnelMethod.addArgument(qmf2.SchemaProperty("udp_sport", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.sch_l2tpCreateTunnelMethod.addArgument(qmf2.SchemaProperty("udp_dport", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.sch_dptcore.addMethod(self.sch_l2tpCreateTunnelMethod)

        self.sch_l2tpDestroyTunnelMethod = qmf2.SchemaMethod("l2tpDestroyTunnel", desc='destroy L2TP tunnel')
        self.sch_l2tpDestroyTunnelMethod.addArgument(qmf2.SchemaProperty("tunnel_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.sch_dptcore.addMethod(self.sch_l2tpDestroyTunnelMethod)

        self.sch_l2tpCreateSessionMethod = qmf2.SchemaMethod("l2tpCreateSession", desc='create L2TP session')
        self.sch_l2tpCreateSessionMethod.addArgument(qmf2.SchemaProperty("name", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_l2tpCreateSessionMethod.addArgument(qmf2.SchemaProperty("tunnel_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.sch_l2tpCreateSessionMethod.addArgument(qmf2.SchemaProperty("session_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.sch_l2tpCreateSessionMethod.addArgument(qmf2.SchemaProperty("peer_session_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.sch_dptcore.addMethod(self.sch_l2tpCreateSessionMethod)

        self.sch_l2tpDestroySessionMethod = qmf2.SchemaMethod("l2tpDestroySession", desc='destroy L2TP session')
        self.sch_l2tpDestroySessionMethod.addArgument(qmf2.SchemaProperty("name", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_l2tpDestroySessionMethod.addArgument(qmf2.SchemaProperty("tunnel_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.sch_l2tpDestroySessionMethod.addArgument(qmf2.SchemaProperty("session_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.sch_dptcore.addMethod(self.sch_l2tpDestroySessionMethod)

        self.sch_dptcore_vethLinkCreateMethod = qmf2.SchemaMethod("vethLinkCreate", desc='create a veth pair with devname1 and devname2')
        self.sch_dptcore_vethLinkCreateMethod.addArgument(qmf2.SchemaProperty("devname1", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_dptcore_vethLinkCreateMethod.addArgument(qmf2.SchemaProperty("devname2", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_dptcore.addMethod(self.sch_dptcore_vethLinkCreateMethod)

        self.sch_dptcore_vethLinkDestroyMethod = qmf2.SchemaMethod("vethLinkDestroy", desc='destroy a link identified by devname')
        self.sch_dptcore_vethLinkDestroyMethod.addArgument(qmf2.SchemaProperty("devname", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_dptcore.addMethod(self.sch_dptcore_vethLinkDestroyMethod)

        self.sch_dptcore_linkAddIPMethod = qmf2.SchemaMethod("linkAddIP", desc='assign an IP address to devname')
        self.sch_dptcore_linkAddIPMethod.addArgument(qmf2.SchemaProperty("devname", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_dptcore_linkAddIPMethod.addArgument(qmf2.SchemaProperty("ipaddr", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_dptcore.addMethod(self.sch_dptcore_linkAddIPMethod)

        self.sch_dptcore_linkDelIPMethod = qmf2.SchemaMethod("linkDelIP", desc='remove an IP address from devname')
        self.sch_dptcore_linkDelIPMethod.addArgument(qmf2.SchemaProperty("devname", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_dptcore_linkDelIPMethod.addArgument(qmf2.SchemaProperty("ipaddr", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.sch_dptcore.addMethod(self.sch_dptcore_linkDelIPMethod)

        self.agentSession.registerSchema(self.sch_dptcore)

        
        
    def method(self, handle, methodName, args, subtypes, addr, userId):
        try:
            if methodName == "test":
                handle.addReturnArgument("outstring", "an output string ...")
                self.agentSession.methodSuccess(handle)
                
            elif methodName == "vethLinkCreate":
                self.dptcore.vethLinkCreate(devname1=handle.getArguments()['devname1'], devname2=handle.getArguments()['devname2'])
                self.agentSession.methodSuccess(handle)
                
            elif methodName == "vethLinkDestroy":
                self.dptcore.vethLinkDestroy(handle.getArguments()['devname'])
                self.agentSession.methodSuccess(handle)
                
            elif methodName == "linkAddIP":
                try:
                    self.dptcore.addIP(handle.getArguments()['devname'], handle.getArguments()['ipaddr'])
                    self.agentSession.methodSuccess(handle)
                except KeyError:
                    self.agentSession.raiseException(handle, "device not found")

            elif methodName == "linkDelIP":
                try:
                    self.dptcore.delIP(handle.getArguments()['devname'], handle.getArguments()['ipaddr'])
                    self.agentSession.methodSuccess(handle)
                except KeyError:
                    self.agentSession.raiseException(handle, "device not found")
                                
            elif methodName == "l2tpCreateTunnel":
                self.dptcore.l2tpCreateTunnel(tunnel_id=handle.getArguments()['tunnel_id'],
                                              peer_tunnel_id=handle.getArguments()['peer_tunnel_id'], 
                                              remote=handle.getArguments()['remote'],
                                              local=handle.getArguments()['local'],
                                              udp_sport=handle.getArguments()['udp_sport'],
                                              udp_dport=handle.getArguments()['udp_dport'])
                self.agentSession.methodSuccess(handle)
                
            elif methodName == "l2tpDestroyTunnel":
                self.dptcore.l2tpDestroyTunnel(tunnel_id=handle.getArguments()['tunnel_id'])
                self.agentSession.methodSuccess(handle)
                
            elif methodName == "l2tpCreateSession":
                self.dptcore.l2tpCreateSession(name=handle.getArguments()['name'],
                                               tunnel_id=handle.getArguments()['tunnel_id'],
                                               session_id=handle.getArguments()['session_id'],
                                               peer_session_id=handle.getArguments()['peer_session_id'])
                self.agentSession.methodSuccess(handle)
                
            elif methodName == "l2tpDestroySession":
                self.dptcore.l2tpDestroySession(tunnel_id=handle.getArguments()['tunnel_id'],
                                               session_id=handle.getArguments()['session_id'])
                self.agentSession.methodSuccess(handle)
                
            else:
                self.agentSession.raiseException(handle, "blubber")
        except:
            self.agentSession.raiseException(handle, "something failed, but we do not know, what ...")



class DptCoreException(BaseException):
    def __init__(self, serror):
        self.serror = serror
    def __str__(self):
        return "<DptCoreException => serror: " + self.serror + " >"


class DptCoreEvent(object):
    def __init__(self, source, type):
        self.source = source
        self.type = type


class DptCore(object):
    def __init__(self, broker_url="localhost"):
        (self.pipein, self.pipeout) = os.pipe()
        self.ipdb = IPDB()
        self.events = []
        self.qmf    = {}
        try:
            self.qmf['connection'] = cqpid.Connection(broker_url)
            self.qmf['connection'].open()
            self.qmf['session'] = qmf2.AgentSession(self.qmf['connection'], "{interval:10, reconnect:True}")
            self.qmf['session'].setVendor('bisdn.de')
            self.qmf['session'].setProduct('qmf_dptcore')
            self.qmf['session'].open()
            self.qmf['handler'] = DptCoreQmfAgentHandler(self, self.qmf['session'])
        except:
            raise
        self.l2tpTunnels = {}
        

    def add_event(self, event):
        self.events.append(event)
        os.write(self.pipeout, str(event.type))

    
    def run(self):
        # start QMF agent handler thread 
        self.qmf['handler'].start()
        
        self.ep = select.epoll()
        self.ep.register(self.pipein, select.EPOLLET | select.EPOLLIN)
                
        cont = True
        while cont:
            try :
                self.ep.poll(timeout=-1, maxevents=1)
                evType = os.read(self.pipein, 256)
                                
                for event in self.events:
                    if event.type == EVENT_QUIT:
                        cont = False
                else:
                    self.events = []
            except KeyboardInterrupt:
                self.add_event(DptCoreEvent(self, EVENT_QUIT))

        # stop QMF agent handler thread 
        self.qmf['handler'].cancel()
        # close QMF AgentSession
        self.qmf['session'].close()
        # close QMF connection
        self.qmf['connection'].close()


    def vethLinkCreate(self, **kwargs):
        """
        creates a single veth pair
        
        kwargs parameters:
        devname1
        devname2
        """
        scmd = '/sbin/ip link add ' + \
                'name ' + kwargs['devname1'] + ' ' + \
                'type veth peer ' + \
                'name ' + kwargs['devname2'] 
        subprocess.call(scmd.split())
        for devname in ( kwargs ['devname1'], kwargs['devname2'] ):
            scmd = '/sbin/ip link set up dev ' + devname
            subprocess.call(scmd.split())
    
    
    def vethLinkDestroy(self, devname):
        """
        destroys an interface
        """
        scmd = '/sbin/ip link del dev ' + devname
        subprocess.call(scmd.split())
    
    
    def addIP(self, devname, ipaddr):
        """
        add an IP address to a link
        """
        scmd = '/sbin/ip addr add ' + ipaddr + ' dev ' + devname
        subprocess.call(scmd.split())
        #self.ipdb[devname].add_ip(ipaddr)
        #self.ipdb[devname].commit()
        
        
    def delIP(self, devname, ipaddr):
        """
        delete an IP address from a link
        """
        scmd = '/sbin/ip addr del ' + ipaddr + ' dev ' + devname
        subprocess.call(scmd.split())
        #self.ipdb[devname].del_ip(ipaddr)
        #self.ipdb[devname].commit()
        

    def l2tpTunnelCreate(self, **kwargs):
        """
        create an L2TP tunnel
        
        encap is always UDP, parameters:
            kwargs['tunnel_id']
            kwargs['peer_tunnel_id']
            kwargs['local']
            kwargs['remote']
            kwargs['udp_sport']
            kwargs['udp_dport']
        """
        if not 'tunnel_id' in kwargs:
            raise DptCoreException("parameter tunnel_id is missing")
        if not 'peer_tunnel_id' in kwargs:
            raise DptCoreException("parameter peer_tunnel_id is missing")
        if not 'local' in kwargs:
            raise DptCoreException("parameter local is missing")
        if not 'remote' in kwargs:
            raise DptCoreException("parameter remote is missing")
        if not 'udp_sport' in kwargs:
            raise DptCoreException("parameter udp_sport is missing")
        if not 'udp_dport' in kwargs:
            raise DptCoreException("parameter udp_dport is missing")

        if kwargs['tunnel_id'] in self.l2tpTunnels:
            raise DptCoreException("tunnel exists")
        self.l2tpTunnels[kwargs['tunnel_id']] = L2tpTunnel(kwargs['tunnel_id'], 
                                                           kwargs['peer_tunnel_id'], 
                                                           kwargs['local'],
                                                           kwargs['remote'],  
                                                           kwargs['udp_sport'], 
                                                           kwargs['udp_dport'])
        self.l2tpTunnels[kwargs['tunnel_id']].create()
        return self.l2tpTunnels[kwargs['tunnel_id']]



    def l2tpTunnelDestroy(self, **kwargs):
        """
        destroy an L2TP tunnel
        """
        if not 'tunnel_id' in kwargs:
            raise DptCoreException("parameter tunnel_id is missing")
        self.l2tpTunnels[kwargs['tunnel_id']].destroy()
        del self.l2tpTunnels[kwargs['tunnel_id']]
        
        

    def l2tpSessionCreate(self, **kwargs):
        """
        create an L2TP session
        """
        if not 'name' in kwargs:
            raise DptCoreException("parameter name is missing")
        if not 'tunnel_id' in kwargs:
            raise DptCoreException("parameter tunnel_id is missing")
        if not 'session_id' in kwargs:
            raise DptCoreException("parameter session_id is missing")
        if not 'peer_session_id' in kwargs:
            raise DptCoreException("parameter peer_session_id is missing")

        if kwargs['tunnel_id'] in self.l2tpTunnels:
            raise DptCoreException("tunnel exists")

        return self.l2tpTunnels[kwargs['tunnel_id']].addSession(kwargs['name'], kwargs['session_id'], kwargs['peer_session_id'])


    def l2tpSessionDestroy(self, **kwargs):
        """
        create an L2TP session
        """
        if not 'tunnel_id' in kwargs:
            raise DptCoreException("parameter tunnel_id is missing")
        if not 'session_id' in kwargs:
            raise DptCoreException("parameter session_id is missing")

        if not kwargs['tunnel_id'] in self.l2tpTunnels:
            raise DptCoreException("tunnel not found")
        
        self.l2tpTunnels[kwargs['tunnel_id']].delSession(kwargs['session_id'])





if __name__ == "__main__":
    DptCore().run()

    
