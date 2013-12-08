#!/usr/bin/python

from qmf.console import Session
import time
import sys

try:
	sess = Session()
	sess.addBroker("192.168.20.10")
except:
	raise

hgwcores = sess.getObjects(_class="hgwcore", _package="de.bisdn.proact")

for hgwcore in hgwcores:
	print hgwcore
	for m in hgwcore.getMethods():
		print m

hgwcore = hgwcores[0]

if sys.argv[1] == 'start':
	print hgwcore.startOpenVpn()

if sys.argv[1] == 'stop':
	print hgwcore.stopOpenVpn()

