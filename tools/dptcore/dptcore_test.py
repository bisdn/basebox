#!/usr/bin/python

from qmf.console import Session
import time

sess = Session()

try:
        broker = sess.addBroker("amqp://172.16.250.65:5672")
except:
        print "Connection failed"
        exit(1)

dptcores = sess.getObjects(_class="dptcore", _package="de.bisdn.dptcore")

for dptcore in dptcores:
	print dptcore
        props = dptcore.getProperties()
        for prop in props:
                print prop
        stats = dptcore.getStatistics()
        for stat in stats:
                print stat
        methods = dptcore.getMethods()
        for method in methods:
                print method


if len(dptcores) == 0:
	exit(0)


#for dptcore in dptcores:
#    print dptcore.test("blub")

dptcore = dptcores[0]

dptcore.vethLinkCreate('veth0', 'veth1')
dptcore.linkAddIP('veth1', '3000::10', 64, '3000::1')

time.sleep(2)

cpeTunnelID=10
vhsTunnelID=10
cpeSessionID=1
vhsSessionID=1
cpeIP='3000::1'
vhsIP='3000::2'
cpeUdpPort=6000
vhsUdpPort=6001

print "CREATE TUNNEL"
dptcore.l2tpCreateTunnel(cpeTunnelID, vhsTunnelID, vhsIP, cpeIP, cpeUdpPort, vhsUdpPort)
print "CREATE SESSION"
dptcore.l2tpCreateSession('l2tpeth10', cpeTunnelID, cpeSessionID, vhsSessionID)

# self.getDptCoreInstance().l2tpCreateTunnel(tunnel_id, peer_tunnel_id, remote, local, udp_sport, udp_dport)
# self.getDptCoreInstance().l2tpCreateSession(name, tunnel_id, session_id, peer_session_id)

time.sleep(8)

print "DESTROY SESSION"
dptcore.l2tpDestroySession('l2tpeth10', cpeTunnelID, cpeSessionID)
print "DESTROY TUNNEL"
dptcore.l2tpDestroyTunnel(cpeTunnelID)

time.sleep(2)

dptcore.linkDelIP('veth1', '3000::1/64')
dptcore.vethLinkDestroy('veth0')

sess.close()

# <Tunnel devname:veth1 cpeTunnelID:10 vhsTunnelID:10 cpeSessionID:1 vhsSessionID:1 cpeIP:3000:1:0:ffc2::2 vhsIP:1000::100 cpeUdpPort:6000 vhsUdpPort:6001 >
