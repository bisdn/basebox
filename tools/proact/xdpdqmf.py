#!/usr/bin/python

from qmf.console import Session
import time
import sys

_class='xdpd'
_package='de.bisdn.xdpd'
xdpdID='dpt-blue-home-proact'


if len(sys.argv) < 2:
	raise BaseException('invalid parameters')

try:
	sess = Session()
	sess.addBroker('127.0.0.1')
except:
	raise



# lsiCreate(dpid, dpname, ofversion, ntables, ctlaf, ctladdr, ctlport, reconnect)
if sys.argv[1] == 'lsi':
	# add LSI
	if sys.argv[2] == 'add':
		if len(sys.argv) < 9:
			raise BaseException('invalid parameters')

		(xdpdID, dpid, dpname, ofversion, ntables, ctlaf, ctladdr, ctlport, reconnect, ) = sys.argv[3:]

		for lsi in sess.getObjects(_class='lsi', _package=_package):
			if lsi.dpid == dpid:
				break
		else:
			for xdpd in sess.getObjects(_class='xdpd', _package=_package):
				if xdpd.xdpdID == xdpdID:
					break
			else:
				raise BaseException('xdpdID <%s> not found' % xdpdID)
	
			print xdpd.lsiCreate(int(dpid), dpname, int(ofversion), int(ntables), int(ctlaf), ctladdr, int(ctlport), int(reconnect))

	# delete LSI
	elif sys.argv[2] == 'delete':
		if len(sys.argv) < 4:
			raise BaseException('invalid parameters')

		(dpid, ) = sys.argv[3:]

		for xdpd in sess.getObjects(_class=_class, _package=_package):
			if xdpd.xdpdID == xdpdID:
				print xdpd.lsiDestroy(int(dpid))
				break

	# attach port
	elif sys.argv[2] == 'attach':
		if len(sys.argv) < 5:
			raise BaseException('invalid parameters')

		(dpid, devname, ) = sys.argv[3:]
		for lsi in sess.getObjects(_class='lsi', _package=_package):
			if lsi.dpid == int(dpid):
				print lsi.portAttach(int(dpid), devname)
				break
		else:
			raise BaseException('LSI with dpid <%s> not found' % str(dpid))


	# detach port
	elif sys.argv[2] == 'detach':
		if len(sys.argv) < 5:
			raise BaseException('invalid parameters')

		(dpid, devname, ) = sys.argv[3:]
		for lsi in sess.getObjects(_class='lsi', _package=_package):
			if lsi.dpid == int(dpid):
				print lsi.portDetach(int(dpid), devname)
				break
		else:
			raise BaseException('LSI with dpid <%s> not found' % str(dpid))



		

