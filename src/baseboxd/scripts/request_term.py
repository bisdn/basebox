import requests
import json

payload = {'command': 'drop_gre_term_in4', 'dpid': 256, 'term_id': 1}

url = 'http://127.0.0.1:8080'
r = requests.post(url, data=json.dumps(payload))
print r.status_code
#print r.content

payload = {'command': 'add_port', 'dpid': 256, 'vid': 1, 'portno': 3, 'tagged': False}
url = 'http://127.0.0.1:8080'
r = requests.post(url, data=json.dumps(payload))
print r.status_code
#print r.content

