import ethcore
import grecore
import time
import sys
import SimpleHTTPServer
import SocketServer
import logging
import cgi
import json
import urlparse

PORT=8080


print "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX [1]"

#print "SWITCHES: " + str(ethcore.get_switches())

#ethcore.add_vlan(dpid=256, vid=1)
#ethcore.add_port(dpid=256, vid=1, portno=1, tagged=False)
#ethcore.add_port(dpid=256, vid=1, portno=2, tagged=False)

#print "VLANS: " + str(ethcore.get_vlans(dpid=256))

print "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX [2]"


#while True:
#	grecore.add_gre_term_in4(dpid=256, term_id=1, gre_portno=3, local="10.1.1.1", remote="10.1.1.10", gre_key=0x11223344)
#	print "GREv4: " + str(grecore.get_gre_terms_in4(dpid=256))
#	print "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX [3]"
#
#	sys.stdout.flush()
#	#time.sleep(10)
#
#	#grecore.drop_gre_term_in4(dpid=256, term_id=1)
#	#print "GREv4: " + str(grecore.get_gre_terms_in4(dpid=256))
#	#print "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX [4]"
#
#	#sys.stdout.flush()
#	#time.sleep(10)
#
#	sys.exit(0)

class ServerHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    def do_GET(self):
        logging.warning("======= GET STARTED =======")
        logging.warning(self.headers)
	self.send_response(204)
	self.end_headers()

    def do_POST(self):
        logging.warning("======= POST STARTED =======")
        logging.warning(self.headers)
        logging.warning("======= POST VALUES =======")
        length = int(self.headers['Content-Length'])
	logging.warning("length=%d" % (length))
        #post_data = urlparse.parse_qs(self.rfile.read(length).decode('utf-8'))
        post = json.loads(self.rfile.read(length).decode('utf-8'))
	logging.warning("post-data=%s" % (post))

	if not 'command' in post:
	    self.send_response(404)
	    self.end_headers()

        elif post['command'] == 'add_vlan':
   	    post.pop('command', None)
	    try:
		ethcore.add_vlan(**post)
	        self.send_response(200)
	        self.end_headers()
	    except:
	        self.send_response(503)
	        self.end_headers()

        elif post['command'] == 'drop_vlan':
   	    post.pop('command', None)
	    try:
		ethcore.drop_vlan(**post)
	        self.send_response(200)
	        self.end_headers()
	    except:
	        self.send_response(503)
	        self.end_headers()

        elif post['command'] == 'add_port':
   	    post.pop('command', None)
	    try:
		ethcore.add_port(**post)
	        self.send_response(200)
	        self.end_headers()
	    except:
	        self.send_response(503)
	        self.end_headers()

        elif post['command'] == 'drop_port':
   	    post.pop('command', None)
	    try:
		ethcore.drop_port(**post)
	        self.send_response(200)
	        self.end_headers()
	    except:
	        self.send_response(503)
	        self.end_headers()

        elif post['command'] == 'add_gre_term_in4':
   	    post.pop('command', None)
	    try:
		grecore.add_gre_term_in4(**post)
	        self.send_response(200)
	        self.end_headers()
	    except:
	        self.send_response(503)
	        self.end_headers()

        elif post['command'] == 'drop_gre_term_in4':
   	    post.pop('command', None)
	    logging.warning("post-data=%s" % (post))
	    try:
		grecore.drop_gre_term_in4(**post)
	        self.send_response(200)
	        self.end_headers()
	    except:
	        self.send_response(503)
	        self.end_headers()

        else:
	    self.send_response(404)
	    self.end_headers()


SocketServer.TCPServer.allow_reuse_address = True
Handler = ServerHandler
httpd = SocketServer.TCPServer(("", PORT), Handler)
try:
    httpd.serve_forever()
except KeyboardInterrupt:
    httpd.server_close()
