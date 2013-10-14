#!/usr/bin/python

from qmf.console import Session
import time

sess = Session()

try:
        broker = sess.addBroker("amqp://localhost:5672")
except:
        print "Connection failed"
        exit(1)

dptcores = sess.getObjects(_class="dptcore", _package="de.bisdn.homegw")

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


dptcore = dptcores[0]
print dptcore.test("blub")
