#!/bin/bash

# This is a testing script for bridge setup and teardown

BRIDGE=${BRIDGE:-swbridge}
PORTA=${PORTA:-port7}
PORTB=${PORTB:-port8}

function bridge_setup {
  ip link add name ${BRIDGE} type bridge vlan_filtering 1 vlan_protocol $1
  ip link set ${BRIDGE} up
}

function enslave {
  # port 1
  ip link set ${PORTA} master ${BRIDGE}
  ip link set ${PORTA} up

  # port 2
  ip link set ${PORTB} master ${BRIDGE}
  ip link set ${PORTB} up
}

function vlans {
  bridge vlan add vid 100 dev ${PORTA}
  bridge vlan add vid 100 dev ${PORTB}
}

function teardown {
  ip link del ${BRIDGE}
}

teardown

# Initial bridge setup, interface enslavement and VLAN configuration
bridge_setup 802.1ad
enslave
vlans

read -p "First setup: Press enter to continue"

# Delete the bridge
teardown

read -p "Second setup: Press enter to continue"

# Reconfiguration of the bridge, state in OFDPA must be the same as before
bridge_setup 802.1q
enslave
vlans
