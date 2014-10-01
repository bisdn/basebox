import requests
import json

payload = {'command': 'drop_port', 'dpid': 256, 'vid': 1, 'portno': 3}
url = 'http://127.0.0.1:8080'
r = requests.post(url, data=json.dumps(payload))
print r.status_code
#print r.content


payload = {'command': 'add_gre_term_in4', 'dpid': 256, 'term_id': 1, 'gre_portno': 3, 'local': "10.1.1.1", 'remote': "10.1.1.10", 'gre_key': 0x11223344}
url = 'http://127.0.0.1:8080'
r = requests.post(url, data=json.dumps(payload))
print r.status_code
print r.content
