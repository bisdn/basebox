#!/usr/bin/python

from qmf.console import Session
import time

sess = Session()

try:
        broker = sess.addBroker("amqp://localhost:5672")
except:
        print "Connection failed"
        exit(1)

vhsgws = sess.getObjects(_class="gateway", _package="de.bisdn.vhsgw")

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

#print vhsgw.test("blub")
print vhsgw.vhsAttach('l2tp', 10, 20, '10.2.2.2', 6000)

time.sleep(4)

print vhsgw.vhsDetach('l2tp', 10, 20)



#vhsgw.vethLinkCreate('blab', 'blub')
#vhsgw.linkAddIP('blub', '3000::1/64')

#time.sleep(8)

#vhsgw.linkDelIP('blub', '3000::1/64')
#vhsgw.vethLinkDestroy('blab')

#sess.close()


