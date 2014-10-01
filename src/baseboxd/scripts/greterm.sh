#!/bin/bash

IP=/sbin/ip
BRCTL=/sbin/brctl

case "$CMD" in
INIT)
	echo "${IP} link add dev ${GREIFACE} type gretap local ${GRELOCAL} remote ${GREREMOTE} csum key ${GREKEY}"
	${IP} link add dev ${GREIFACE} type gretap local ${GRELOCAL} remote ${GREREMOTE} csum key ${GREKEY}
	echo "${IP} link add dev ${BRIDGE} type bridge"
	${IP} link add dev ${BRIDGE} type bridge	
	echo "${BRCTL} stp ${BRIDGE} on"
	${BRCTL} stp ${BRIDGE} on
	echo "${BRCTL} addif ${BRIDGE} ${GREIFACE}"
	${BRCTL} addif ${BRIDGE} ${GREIFACE}	
	echo "${IP} link set up dev ${GREIFACE}"
	${IP} link set up dev ${GREIFACE}
	echo "${BRCTL} addif ${BRIDGE} ${IFACE}"
	${BRCTL} addif ${BRIDGE} ${IFACE}	
	echo "${IP} link set up dev ${IFACE}"
	${IP} link set up dev ${IFACE}
	echo "${IP} link set up dev ${BRIDGE}"
	${IP} link set up dev ${BRIDGE}
	;;
TERM)
	echo "${IP} link set down dev ${BRIDGE}"
	${IP} link set down dev ${BRIDGE}
	echo "${IP} link set down dev ${IFACE}"
	${IP} link set down dev ${IFACE}
	echo "${BRCTL} delif ${BRIDGE} ${IFACE}"	
	${BRCTL} delif ${BRIDGE} ${IFACE}	
	echo "${IP} link set down dev ${GREIFACE}"
	${IP} link set down dev ${GREIFACE}
	echo "${BRCTL} delif ${BRIDGE} ${GREIFACE}"	
	${BRCTL} delif ${BRIDGE} ${GREIFACE}	
	echo "${BRCTL} delbr ${BRIDGE}"
	${BRCTL} delbr ${BRIDGE}
	echo "${IP} link del dev ${GREIFACE}"
	${IP} link del dev ${GREIFACE}
	;;
esac

