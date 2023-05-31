import requests
import json

url = 'http://137.99.0.4/nodes';
v_threshold=2.4
#url = 'perf.txt';

r = requests.get(url);
nodes = r.json();

for node in nodes:
    strid=str(node["_id"]);
    if strid=='1':
        continue
    if node["lifetime"]<0:
        continue
    print(strid)
    r = requests.put(url+"/"+strid+"/led/1");

