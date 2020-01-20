#!/bin/bash

PORTA=${PORTA:-port31}
PORTB=${PORTB:-port32}
PORTC=${PORTC:-port33}
PORTD=${PORTD:-port34}
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
  bridge vlan add vid 123 dev ${PORTC}
  bridge vlan add vid 124 dev ${PORTD}

  # Access port vids
  bridge vlan add vid 123 dev ${PORTA} pvid
  bridge vlan add vid 124 dev ${PORTB} pvid
}

setup
