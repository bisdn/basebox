import requests
import json

payload = {'command': 'add_vlan', 'dpid': 256, 'vid': 1}
url = 'http://127.0.0.1:8080'
r = requests.post(url, data=json.dumps(payload))
print r.status_code
#print r.content

payload = {'command': 'add_port', 'dpid': 256, 'vid': 1, 'portno': 1, 'tagged': False}
url = 'http://127.0.0.1:8080'
r = requests.post(url, data=json.dumps(payload))
print r.status_code
#print r.content

payload = {'command': 'add_port', 'dpid': 256, 'vid': 1, 'portno': 2, 'tagged': False}
url = 'http://127.0.0.1:8080'
r = requests.post(url, data=json.dumps(payload))
print r.status_code
#print r.content

payload = {'command': 'drop_port', 'dpid': 256, 'vid': 1, 'portno': 3}
url = 'http://127.0.0.1:8080'
r = requests.post(url, data=json.dumps(payload))
print r.status_code
#print r.content
