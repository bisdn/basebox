#!/usr/bin/python

from qmf.console import Session
import time

sess = Session()

try:
        broker = sess.addBroker("amqp://localhost:5672")
except:
        print "Connection failed"
        exit(1)

vhsgws = sess.getObjects(_class="vhscore", _package="de.bisdn.proact")

for vhsgw in vhsgws:    
    props = vhsgw.getProperties()
    for prop in props:
            print prop
    stats = vhsgw.getStatistics()
    for stat in stats:
            print stat
    methods = vhsgw.getMethods()
    for method in methods:
            print method


if len(vhsgws) == 0:
	exit(0)


#for vhsgw in vhsgws:
#    print vhsgw.test("blub")

vhsgw = vhsgws[0]

print vhsgw

print vhsgw.vhsCoreID

print vhsgw.test("blub")

print vhsgw.userSessionAdd('you@somewhere.com', '10.1.1.1', '86400')

print vhsgw.userSessionDel('you@somewhere.com', '10.1.1.1')

sess.close()


