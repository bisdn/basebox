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
