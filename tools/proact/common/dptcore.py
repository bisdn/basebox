#!/usr/bin/python

import sys
sys.path.append('../..')
print sys.path

import proact.common.basecore
import proact.common.l2tp
import subprocess
import qmf2




class DptCoreException(BaseException):
    def __init__(self, serror):
        self.serror = serror
    def __str__(self):
        return "<DptCoreException => serror: " + self.serror + " >"



class DptCoreQmfAgentHandler(proact.common.basecore.BaseCoreQmfAgentHandler):
    def __init__(self, dptCore, agentSession):
        proact.common.basecore.BaseCoreQmfAgentHandler.__init__(self, agentSession)
        self.dptCore = dptCore
        self.qmfDptCore = {}
        self.qmfDptCore['data'] = qmf2.Data(self.qmfSchemaDptCore)
        self.qmfDptCore['data'].dptCoreID = dptCore.dptCoreID
        self.qmfDptCore['addr'] = self.agentSess.addData(self.qmfDptCore['data'], 'dptcore')
        
    def setSchema(self):  
        self.qmfSchemaDptCore = qmf2.Schema(qmf2.SCHEMA_TYPE_DATA, "de.bisdn.proact.dptcore", "dptcore")
        self.qmfSchemaDptCore.addProperty(qmf2.SchemaProperty("dptCoreID", qmf2.SCHEMA_DATA_STRING))
        
        self.sch_l2tpResetMethod = qmf2.SchemaMethod("l2tpReset", desc='destroy all L2TP tunnels')
        self.qmfSchemaDptCore.addMethod(self.sch_l2tpResetMethod)

        self.qmfSchemaMethodL2tpCreateTunnel = qmf2.SchemaMethod("l2tpCreateTunnel", desc='create L2TP tunnel')
        self.qmfSchemaMethodL2tpCreateTunnel.addArgument(qmf2.SchemaProperty("tunnel_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodL2tpCreateTunnel.addArgument(qmf2.SchemaProperty("peer_tunnel_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodL2tpCreateTunnel.addArgument(qmf2.SchemaProperty("remote", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodL2tpCreateTunnel.addArgument(qmf2.SchemaProperty("local", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodL2tpCreateTunnel.addArgument(qmf2.SchemaProperty("udp_sport", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodL2tpCreateTunnel.addArgument(qmf2.SchemaProperty("udp_dport", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.qmfSchemaDptCore.addMethod(self.qmfSchemaMethodL2tpCreateTunnel)

        self.qmfSchemaMethodL2tpDestroyTunnel = qmf2.SchemaMethod("l2tpDestroyTunnel", desc='destroy L2TP tunnel')
        self.qmfSchemaMethodL2tpDestroyTunnel.addArgument(qmf2.SchemaProperty("tunnel_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.qmfSchemaDptCore.addMethod(self.qmfSchemaMethodL2tpDestroyTunnel)

        self.qmfSchemaMethodL2tpCreateSession = qmf2.SchemaMethod("l2tpCreateSession", desc='create L2TP session')
        self.qmfSchemaMethodL2tpCreateSession.addArgument(qmf2.SchemaProperty("name", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodL2tpCreateSession.addArgument(qmf2.SchemaProperty("tunnel_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodL2tpCreateSession.addArgument(qmf2.SchemaProperty("session_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodL2tpCreateSession.addArgument(qmf2.SchemaProperty("peer_session_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.qmfSchemaDptCore.addMethod(self.qmfSchemaMethodL2tpCreateSession)

        self.qmfSchemaMethodL2tpDestroySession = qmf2.SchemaMethod("l2tpDestroySession", desc='destroy L2TP session')
        self.qmfSchemaMethodL2tpDestroySession.addArgument(qmf2.SchemaProperty("name", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodL2tpDestroySession.addArgument(qmf2.SchemaProperty("tunnel_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodL2tpDestroySession.addArgument(qmf2.SchemaProperty("session_id", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.qmfSchemaDptCore.addMethod(self.qmfSchemaMethodL2tpDestroySession)

        self.qmfSchemaMethodVethLinkCreate = qmf2.SchemaMethod("vethLinkCreate", desc='create a veth pair with devname1 and devname2')
        self.qmfSchemaMethodVethLinkCreate.addArgument(qmf2.SchemaProperty("devname1", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodVethLinkCreate.addArgument(qmf2.SchemaProperty("devname2", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaDptCore.addMethod(self.qmfSchemaMethodVethLinkCreate)

        self.qmfSchemaMethodVethLinkDestroy = qmf2.SchemaMethod("vethLinkDestroy", desc='destroy a link identified by devname')
        self.qmfSchemaMethodVethLinkDestroy.addArgument(qmf2.SchemaProperty("devname", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaDptCore.addMethod(self.qmfSchemaMethodVethLinkDestroy)

        self.qmfSchemaMethodLinkAddIP = qmf2.SchemaMethod("linkAddIP", desc='assign an IP address to devname')
        self.qmfSchemaMethodLinkAddIP.addArgument(qmf2.SchemaProperty("devname", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodLinkAddIP.addArgument(qmf2.SchemaProperty("ipaddr", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodLinkAddIP.addArgument(qmf2.SchemaProperty("prefixlen", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodLinkAddIP.addArgument(qmf2.SchemaProperty("defroute", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaDptCore.addMethod(self.qmfSchemaMethodLinkAddIP)

        self.qmfSchemaMethodLinkDelIP = qmf2.SchemaMethod("linkDelIP", desc='remove an IP address from devname')
        self.qmfSchemaMethodLinkDelIP.addArgument(qmf2.SchemaProperty("devname", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodLinkDelIP.addArgument(qmf2.SchemaProperty("ipaddr", qmf2.SCHEMA_DATA_STRING, direction=qmf2.DIR_IN))
        self.qmfSchemaMethodLinkDelIP.addArgument(qmf2.SchemaProperty("prefixlen", qmf2.SCHEMA_DATA_INT, direction=qmf2.DIR_IN))
        self.qmfSchemaDptCore.addMethod(self.qmfSchemaMethodLinkDelIP)

        self.agentSession.registerSchema(self.qmfSchemaDptCore)
        
        
    def method(self, handle, methodName, args, subtypes, addr, userId):
        try:
            print "method: " + str(methodName)

            if methodName == "vethLinkCreate":
                self.dptCore.vethLinkCreate(devname1=args['devname1'], devname2=args['devname2'])
                self.agentSession.methodSuccess(handle)
                
            elif methodName == "vethLinkDestroy":
                self.dptCore.vethLinkDestroy(args['devname'])
                self.agentSession.methodSuccess(handle)
                
            elif methodName == "linkAddIP":
                try:
                    self.dptCore.addIP(args['devname'], args['ipaddr'], args['prefixlen'], args['defroute'])
                    self.agentSession.methodSuccess(handle)
                except KeyError:
                    self.agentSession.raiseException(handle, "device not found")

            elif methodName == "linkDelIP":
                try:
                    self.dptCore.delIP(args['devname'], args['ipaddr'], args['prefixlen'])
                    self.agentSession.methodSuccess(handle)
                except KeyError:
                    self.agentSession.raiseException(handle, "device not found")
                                
            elif methodName == "l2tpCreateTunnel":
                self.dptCore.l2tpTunnelCreate(tunnel_id=args['tunnel_id'],
                                              peer_tunnel_id=args['peer_tunnel_id'], 
                                              remote=args['remote'],
                                              local=args['local'],
                                              udp_sport=args['udp_sport'],
                                              udp_dport=args['udp_dport'])
                self.agentSession.methodSuccess(handle)
                
            elif methodName == "l2tpReset":
                self.dptCore.l2tpReset()
                self.agentSession.methodSuccess(handle)
                
            elif methodName == "l2tpDestroyTunnel":
                self.dptCore.l2tpTunnelDestroy(tunnel_id=args['tunnel_id'])
                self.agentSession.methodSuccess(handle)
                
            elif methodName == "l2tpCreateSession":
                self.dptCore.l2tpSessionCreate(name=args['name'],
                                               tunnel_id=args['tunnel_id'],
                                               session_id=args['session_id'],
                                               peer_session_id=args['peer_session_id'])
                self.agentSession.methodSuccess(handle)
                
            elif methodName == "l2tpDestroySession":
                self.dptCore.l2tpSessionDestroy(tunnel_id=args['tunnel_id'],
                                               session_id=args['session_id'])
                self.agentSession.methodSuccess(handle)
                
            else:
                self.agentSession.raiseException(handle, "blubber")
        except DptCoreException, e:
            print str(e)
            self.agentSession.raiseException(handle, e.serror)
        except proact.common.l2tp.L2tpTunnelException, e:
            print str(e)
            self.agentSession.raiseException(handle, e.serror)
        except proact.common.l2tp.L2tpSessionException, e:
            print str(e)
            self.agentSession.raiseException(handle, e.serror)
        except:
            print "something failed, but we do not know, what ..."
            self.agentSession.raiseException(handle, "something failed, but we do not know, what ...")
            raise





class DptCore(proact.common.basecore.BaseCore):
    """
    a description would be useful here
    """
    def __init__(self, dptCoreID='dptcore-0', brokerUrl="127.0.0.1", **kwargs):
        self.brokerUrl = brokerUrl
        self.dptCoreID = dptCoreID
        self.vendor = kwargs.get('vendor', 'bisdn.de')
        self.product = kwargs.get('product', 'dptcore')
        proact.common.basecore.BaseCore.__init__(self, vendor=self.vendor, product=self.product)
        self.agentHandler = DptCoreQmfAgentHandler(self, self.qmfAgent.agentSess)
        self.l2tpTunnels = {}

    def cleanUp(self):
        self.agentHandler.cancel()
        
    def handleEvent(self, event):
        pass
    
    
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
    
    
    def addIP(self, devname, ipaddr, prefixlen, defroute):
        """
        add an IP address to a link
        """
        scmd = '/sbin/ip addr add ' + ipaddr + '/' + str(prefixlen) + ' dev ' + devname
        subprocess.call(scmd.split())
        scmd = '/sbin/ip route add default via ' + defroute + ' dev ' + devname
        subprocess.call(scmd.split())
        #self.ipdb[devname].add_ip(ipaddr)
        #self.ipdb[devname].commit()
        
        
    def delIP(self, devname, ipaddr, prefixlen):
        """
        delete an IP address from a link
        """
        scmd = '/sbin/ip addr del ' + ipaddr + '/' + str(prefixlen) + ' dev ' + devname
        subprocess.call(scmd.split())
        #self.ipdb[devname].del_ip(ipaddr)
        #self.ipdb[devname].commit()
        
        
    def l2tpReset(self):
        """
        destroy all L2TP tunnels
        """
        for tunnel_id, tun in self.l2tpTunnels.iteritems():
            tun.destroy()
        self.l2tpTunnels = {}
        

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
        self.l2tpTunnels[kwargs['tunnel_id']] = proact.common.l2tp.L2tpTunnel(kwargs['tunnel_id'], 
                                                           kwargs['peer_tunnel_id'], 
                                                           kwargs['local'],
                                                           kwargs['remote'],  
                                                           kwargs['udp_sport'], 
                                                           kwargs['udp_dport'])
        self.l2tpTunnels[kwargs['tunnel_id']].create()
        print "CREATING TUNNEL: " + str(self.l2tpTunnels[kwargs['tunnel_id']])
        return self.l2tpTunnels[kwargs['tunnel_id']]



    def l2tpTunnelDestroy(self, **kwargs):
        """
        destroy an L2TP tunnel
        """
        if not 'tunnel_id' in kwargs:
            raise DptCoreException("parameter tunnel_id is missing")
        if kwargs['tunnel_id'] in self.l2tpTunnels: 
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

        if not kwargs['tunnel_id'] in self.l2tpTunnels:
            raise DptCoreException("tunnel not found")

        sess = self.l2tpTunnels[kwargs['tunnel_id']].addSession(kwargs['name'], kwargs['session_id'], kwargs['peer_session_id'])
        print "CREATING SESSION: " + str(sess)
        return sess


    def l2tpSessionDestroy(self, **kwargs):
        """
        destroy an L2TP session
        """
        if not 'tunnel_id' in kwargs:
            raise DptCoreException("parameter tunnel_id is missing")
        if not 'session_id' in kwargs:
            raise DptCoreException("parameter session_id is missing")

        if not kwargs['tunnel_id'] in self.l2tpTunnels:
            raise DptCoreException("tunnel not found")
        
        self.l2tpTunnels[kwargs['tunnel_id']].delSession(kwargs['session_id'])





class DptCoreProxyException(Exception):
    def __init__(self, serror):
        self.serror = serror
    def __str__(self):
        return '<DptCoreProxyException: ' + str(self.serror) + ' >'


class DptCoreProxy(proact.common.qmfhelper.QmfConsole):    
    def __init__(self, brokerUrl = '127.0.0.1', dptCoreID=''):
        proact.common.qmfhelper.QmfConsole.__init__(self, brokerUrl)

        # identifier of xdpd instance to be used
        #
        self.dptCoreID = dptCoreID
        
        
    def __str__(self): 
        return '<DptCoreProxy dptCoreID: ' + str(self.dptCoreID) + '>'


    def cleanUp(self):
        pass


    def __getDptCoreHandle(self):
        dptCoreHandles = self.getObjects(_class='dptcore', _package='de.bisdn.proact.dptcore')
        for dptCoreHandle in dptCoreHandles:
            print dptCoreHandle
            if dptCoreHandle.dptCoreID == self.dptCoreID:
                return dptCoreHandle
        else:
            raise DptCoreProxyException('DptCore instance for dptCoreID: ' + self.dptCoreID + ' not found on QMF bus')

    def addVethLink(self, devname1, devname2):
        try:
            self.__getDptCoreHandle().vethLinkCreate(devname1, devname2)
        except:
            raise DptCoreProxyException('unable to create veth pair (' + devname1 + ',' + devname2 + ')')

    def delVethLink(self, devname):
        try:
            self.__getDptCoreHandle().vethLinkDestroy(devname)
        except:
            raise DptCoreProxyException('unable to destroy veth pair with devname (' + devname + ')')

    def addAddr(self, devname, ipaddr, prefixlen, defroute):
        try:
            self.__getDptCoreHandle().linkAddIP(devname, ipaddr, prefixlen, defroute)
        except:
            raise DptCoreProxyException('unable to add address ' + str(ipaddr) + '/' + str(prefixlen) + ' on devname (' + devname + ')')

    def delAddr(self, devname, ipaddr, prefixlen):
        try:
            self.__getDptCoreHandle().linkDelIP(devname, ipaddr, prefixlen)
        except:
            raise DptCoreProxyException('unable to delete address ' + str(ipaddr) + '/' + str(prefixlen) + ' on devname (' + devname + ')')

    def addL2tpTunnel(self, tunnel_id, peer_tunnel_id, remote, local, udp_sport, udp_dport):
        try:
            self.__getDptCoreHandle().l2tpCreateTunnel(tunnel_id, peer_tunnel_id, remote, local, udp_sport, udp_dport)
        except:
            raise DptCoreProxyException('unable to add L2TP tunnel with tunnel_id ' + str(tunnel_id) + ' and peer_tunnel_id ' + str(peer_tunnel_id))
    
    def delL2tpTunnel(self, tunnel_id):
        try:
            self.__getDptCoreHandle().l2tpDestroyTunnel(tunnel_id)
        except:
            raise DptCoreProxyException('unable to delete L2TP tunnel with tunnel_id ' + str(tunnel_id))
    
    def addL2tpSession(self, name, tunnel_id, session_id, peer_session_id):
        try:
            self.__getDptCoreHandle().l2tpCreateSession(name, tunnel_id, session_id, peer_session_id)
        except: 
            raise DptCoreProxyException('unable to add L2TP session (' + str(name) + ') with tunnel_id ' + str(tunnel_id) 
                                        + ', session_id ' + str(session_id) + ' and peer_session_id ' + str(peer_session_id))
    
    def delL2tpSession(self, tunnel_id, session_id):
        try:
            self.__getDptCoreHandle().l2tpDestroySession(tunnel_id, session_id)
        except:
            raise DptCoreProxyException('unable to delete L2TP session with tunnel_id ' + str(tunnel_id) + ' and session_id ' + str(session_id))
    
    def resetL2tpState(self):
        try:
            self.__getDptCoreHandle().l2tpReset()
        except:
            raise DptCoreProxyException('unable to reset L2TP state')



if __name__ == "__main__":
    DptCore('dptcore-0', '127.0.0.1').run() 
    
    