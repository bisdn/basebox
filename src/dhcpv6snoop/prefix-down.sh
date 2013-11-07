#!/bin/bash

# copy this script to /var/lib/dhcpv6snoop/ and make it executable
#
#

IPROUTE=/sbin/ip

logger "prefix-down: ${IPROUTE} route del ${PREFIX}/${PREFIXLEN} via ${NEXTHOP} dev ${IFACE}"

${IPROUTE} route del ${PREFIX}/${PREFIXLEN} via ${NEXTHOP} dev ${IFACE}
