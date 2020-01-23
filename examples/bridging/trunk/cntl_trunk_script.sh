#!/bin/bash

# Bridging and Trunking test setup script

# This script configures trunk and access ports on the bridge
# Access Ports: Bridge Interfaces configured with ONLY PVID (native VLAN).
# Trunk Ports: Bridge Interfaces configured with more than one VLAN.

PORTA=${PORTA:-port31} # Access ports
PORTB=${PORTB:-port32} # Access ports
PORTC=${PORTC:-port33} # Trunk ports
PORTD=${PORTD:-port34} # Trunk ports
BRIDGE=${BRIDGE:-swbridge}

function setup() {
  ip link add name ${BRIDGE} type bridge vlan_filtering 1
  ip link set ${BRIDGE} up

  # port 1
  ip link set ${PORTA} master ${BRIDGE}
  ip link set ${PORTA} up

  # port 2
  ip link set ${PORTB} master ${BRIDGE}
  ip link set ${PORTB} up

  # port 3
  ip link set ${PORTC} master ${BRIDGE}
  ip link set ${PORTC} up

  # port 4
  ip link set ${PORTD} master ${BRIDGE}
  ip link set ${PORTD} up

  # Trunk port vids
  bridge vlan add vid 133 dev ${PORTC}
  bridge vlan add vid 134 dev ${PORTD}

  # Access port vids
  bridge vlan add vid 131 dev ${PORTA} pvid
  bridge vlan add vid 132 dev ${PORTB} pvid
}

setup
