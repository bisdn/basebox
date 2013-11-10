#!/bin/bash

# copy this script to /var/lib/dhcpv6snoop/ and make it executable
#
#

IPROUTE=/sbin/ip

logger "prefix-up: ${IPROUTE} route add ${PREFIX}/${PREFIXLEN} via ${NEXTHOP} dev ${IFACE}"

${IPROUTE} route add ${PREFIX}/${PREFIXLEN} via ${NEXTHOP} dev ${IFACE}

