#!/usr/bin/python

import subprocess
import time


class L2tpSessionException(BaseException):
    def __init__(self, serror):
        self.serror = serror
    def __str__(self):
        return "<L2tpSessionException => " + self.serror + " >"


class L2tpSession(object):
    """
    """
    def __init__(self, name, tunnel_id, session_id, peer_session_id):
        self.name = name
        self.tunnel_id = tunnel_id
        self.session_id = session_id
        self.peer_session_id = peer_session_id
        self.created = False

    def __str__(self):
        return "<L2tpSession " + \
            "name: " +str(self.name) + " " + \
            "tunnel_id: " +str(self.tunnel_id) + " " + \
            "session_id: " +str(self.session_id) + " " + \
            "peer_session_id: " +str(self.peer_session_id) + " " + \
            " >"
    
    def create(self):
        """
        """
        scmd = '/sbin/ip l2tp add session ' + \
                    'name ' + str(self.name) + ' ' + \
                    'tunnel_id ' + str(self.tunnel_id) + ' ' + \
                    'session_id ' + str(self.session_id) + ' ' + \
                    'peer_session_id ' + str(self.peer_session_id) + ' '
        print 'calling ' + scmd
        subprocess.call(scmd.split())
        self.created = True
        scmd = '/sbin/ip link set up dev ' + str(self.name)
        print 'calling ' + scmd
        subprocess.call(scmd.split())
    
    def destroy(self):
        """
        """
        scmd = '/sbin/ip l2tp del session ' + \
                    'tunnel_id ' + str(self.tunnel_id) + ' ' + \
                    'session_id ' + str(self.session_id) + ' '
        print 'calling ' + scmd
        subprocess.call(scmd.split())
        self.created = False


class L2tpTunnelException(BaseException):
    def __init__(self, serror):
        self.serror = serror
    def __str__(self):
        return "<L2tpTunnelException => " + self.serror + " >"



class L2tpTunnel(object):
    """
    """
    def __init__(self, tunnel_id, peer_tunnel_id, local, remote, udp_sport, udp_dport):
        self.sessions = {}
        self.created = False
        self.tunnel_id = tunnel_id
        self.peer_tunnel_id = peer_tunnel_id
        self.remote = remote
        self.local = local
        self.udp_sport = udp_sport
        self.udp_dport = udp_dport
    
    def __str__(self):
        return "<L2tpTunnel " + \
            "tunnel_id: " +str(self.tunnel_id) + " " + \
            "peer_tunnel_id: " +str(self.peer_tunnel_id) + " " + \
            "remote: " +str(self.remote) + " " + \
            "local: " +str(self.local) + " " + \
            "udp_sport: " +str(self.udp_sport) + " " + \
            "udp_dport: " +str(self.udp_dport) + " " + \
            " >"
    
    def create(self):
        """
        """
        scmd = '/sbin/ip l2tp add tunnel ' + \
                    'tunnel_id ' + str(self.tunnel_id) + ' ' + \
                    'peer_tunnel_id ' + str(self.peer_tunnel_id) + ' ' + \
                    'remote ' + str(self.remote) + ' ' + \
                    'local ' + str(self.local) + ' ' + \
                    'encap udp ' + \
                    'udp_sport ' + str(self.udp_sport) + ' ' + \
                    'udp_dport ' + str(self.udp_dport) + ' '
        print 'calling ' + scmd 
        subprocess.call(scmd.split())
        self.created = True
    
    def destroy(self):
        """
        """
        for session_id in self.sessions:
            self.sessions[session_id].destroy()
        self.sessions = {}
        scmd = '/sbin/ip l2tp del tunnel tunnel_id ' + str(self.tunnel_id)
        print 'calling ' + scmd
        subprocess.call(scmd.split())
        self.created = False
    
    def getSession(self, session_id):
        """
        """
        if not session_id in self.sessions:
            raise L2tpTunnelException("session not found")
        return self.sessions[session_id]

    def addSession(self, name, session_id, peer_session_id):
        """
        """
        if session_id in self.sessions:
            raise L2tpTunnelException("session exists")
        if not self.created:
            self.create()
        self.sessions[session_id] = L2tpSession(name, self.tunnel_id, session_id, peer_session_id)
        self.sessions[session_id].create()
        return self.sessions[session_id]
    
    def delSession(self, session_id):
        """
        """
        if not session_id in self.sessions:
            raise L2tpTunnelException("session not found")
        self.sessions[session_id].destroy()
        del self.sessions[session_id]
        if len(self.sessions) == 0:
            self.destroy()




if __name__ == "__main__":
    tunnel_id = 1
    peer_tunnel_id = 2
    remote = "10.2.2.20"
    local = "10.1.1.10"
    udp_sport = 5000
    udp_dport = 6000
    tunnel = L2tpTunnel(tunnel_id, peer_tunnel_id, remote, local, udp_sport, udp_dport)

    name = 'l2tpeth0'
    session_id = 1111
    peer_session_id = 2222
    session = tunnel.addSession(name, session_id, peer_session_id)
    
    time.sleep(10)
    
    tunnel.destroy()
    
    