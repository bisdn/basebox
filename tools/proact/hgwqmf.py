#!/usr/bin/python

from qmf.console import Session
import time
import sys

try:
	sess = Session()
	sess.addBroker("192.168.20.10")
except:
	raise

hgwcores = sess.getObjects(_class="hgwcore", _package="de.bisdn.hgwcore")

for hgwcore in hgwcores:
	print hgwcore
	for m in hgwcore.getMethods():
		print m

hgwcore = hgwcores[0]

#if sys.argv[1] == 'start':
#	print ethcore.vlanAdd(256, 20)
#	print ethcore.portAdd(256, 20, 'vnet4', True)
#	print ethcore.portAdd(256, 20, 'vnet6', True)

#if sys.argv[1] == 'stop':
#	print ethcore.portDrop(256, 20, 'vnet4')
#	print ethcore.portDrop(256, 20, 'vnet6')
#	print ethcore.vlanDrop(256, 20)

