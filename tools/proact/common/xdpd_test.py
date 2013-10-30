#!/usr/bin/python

from qmf.console import Session
import time

sess = Session()

try:
        broker = sess.addBroker("amqp://127.0.0.1:5672")
except:
        print "Connection failed"
        exit(1)

xdpds = sess.getObjects(_class="xdpd", _package="de.bisdn.xdpd")

for xdpd in xdpds:
	print xdpd
        props = xdpd.getProperties()
        for prop in props:
                print prop
        stats = xdpd.getStatistics()
        for stat in stats:
                print stat
        methods = xdpd.getMethods()
        for method in methods:
                print method


if len(xdpds) == 0:
	exit(0)


#for dptcore in dptcores:
#    print dptcore.test("blub")

xdpd = xdpds[0]

xdpd.lsiCreate(1000, 'vhs-dp0', 3, 4, 2, '127.0.0.1', 6633, 4)

time.sleep(10)

xdpd.lsiDestroy(1000)

sess.close()

# <Tunnel devname:veth1 cpeTunnelID:10 vhsTunnelID:10 cpeSessionID:1 vhsSessionID:1 cpeIP:3000:1:0:ffc2::2 vhsIP:1000::100 cpeUdpPort:6000 vhsUdpPort:6001 >
