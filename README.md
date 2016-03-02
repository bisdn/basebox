# Usage

## bridge setup

```
ip link add type bridge
echo 1 > /sys/class/net/bridge0/bridge/vlan_filtering
ip link set bridge0 up
```

## adding ports to a switch

```
ip link set port1 master bridge0
ip link set port2 master bridge0
```

## adding vlans to bridge interfaces

```
bridge vlan add vid 2 dev port1
bridge vlan add vid 2 dev port2
```


## adding a specific mac address to a bridge port

```
bridge fdb add 68:05:ca:30:63:69 dev port1 master vlan 1
```

# Notes

If you are looking for older versions of basebox(d) for controlling 
ovs and xdpd, you find those in the swx branch on github [1].

[1] https://github.com/bisdn/basebox/tree/swx
