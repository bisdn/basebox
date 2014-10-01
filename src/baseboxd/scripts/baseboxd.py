import ethcore
import grecore
import time
import sys

print "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX [1]"

print "SWITCHES: " + str(ethcore.get_switches())

ethcore.add_vlan(dpid=256, vid=1)
ethcore.add_port(dpid=256, vid=1, portno=1, tagged=False)
ethcore.add_port(dpid=256, vid=1, portno=2, tagged=False)

print "VLANS: " + str(ethcore.get_vlans(dpid=256))

print "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX [2]"


while True:
	grecore.add_gre_term_in4(dpid=256, term_id=1, gre_portno=3, local="10.1.1.1", remote="10.1.1.10", gre_key=0x11223344)
	print "GREv4: " + str(grecore.get_gre_terms_in4(dpid=256))
	print "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX [3]"

	sys.stdout.flush()
	#time.sleep(10)

	#grecore.drop_gre_term_in4(dpid=256, term_id=1)
	#print "GREv4: " + str(grecore.get_gre_terms_in4(dpid=256))
	#print "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX [4]"

	#sys.stdout.flush()
	#time.sleep(10)

	sys.exit(0)
