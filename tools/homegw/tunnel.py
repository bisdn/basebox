#!/usr/bin/python


class TunnelException(BaseException):
    pass




class Tunnel(object):
    """
    ...
    """
    #self.TUNNEL_DOWN = 'down'
    #self.TUNNEL_UP = 'active'

    def __init__(self, cpeDevname, cpeTunnelID, vhsTunnelID, cpeSessionID, vhsSessionID, cpeIP, vhsIP, cpeUdpPort, vhsUdpPort):
        self.state = 'down' 
        self.cpeDevname = cpeDevname
        self.cpeTunnelID = cpeTunnelID
        self.vhsTunnelID = vhsTunnelID
        self.cpeSessionID = cpeSessionID
        self.vhsSessionID = vhsSessionID
        self.cpeIP = cpeIP
        self.vhsIP = vhsIP
        self.cpeUdpPort = cpeUdpPort
        self.vhsUdpPort = vhsUdpPort
        
    def __str__(self):
        return '<Tunnel devname:'+ self.cpeDevname + \
            ' cpeTunnelID:' + str(self.cpeTunnelID) + ' vhsTunnelID:' + str(self.vhsTunnelID) + \
            ' cpeSessionID:' + str(self.cpeSessionID) + ' vhsSessionID:' + str(self.vhsSessionID) + \
            ' cpeIP:' + self.cpeIP + ' vhsIP:' + self.vhsIP + \
            ' cpeUdpPort:' + str(self.cpeUdpPort) + ' vhsUdpPort:' + str(self.vhsUdpPort) + \
            ' >'
            
            