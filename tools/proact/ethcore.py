#!/usr/bin/python

from qmf.console import Session
import time
import sys

try:
	sess = Session()
	sess.addBroker("127.0.0.1")
except:
	raise

ethcores = sess.getObjects(_class="ethcore", _package="de.bisdn.ethcore")

for ethcore in ethcores:
	print ethcore
	for m in ethcore.getMethods():
		print m

ethcore = ethcores[0]

if sys.argv[1] == 'start':
	print ethcore.vlanAdd(256, 20)
	print ethcore.portAdd(256, 20, 'vnet4', True)
	print ethcore.portAdd(256, 20, 'vnet6', True)

if sys.argv[1] == 'stop':
	print ethcore.portDrop(256, 20, 'vnet4')
	print ethcore.portDrop(256, 20, 'vnet6')
	print ethcore.vlanDrop(256, 20)

