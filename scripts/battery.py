import requests
import json

url = 'http://137.99.0.4/nodes';
v_threshold=2.55
#url = 'perf.txt';

r = requests.get(url);
nodes = r.json();
count = 0;

for node in nodes:
    strid=str(node["_id"]);
    if node["lifetime"]<0:
        continue
    voltage=None
    sensors=node.get("sensors");
    if sensors!=None:
        voltage=sensors.get("bat")
    if voltage!=None:
        voltage=float(voltage)
        print("{} = {}".format(strid,voltage),end='')
        if voltage<v_threshold:
            print(" Low!")
            r = requests.put(url+"/"+strid+"/led/1");
            count += 1
        else:
            print()
            r = requests.put(url+"/"+strid+"/led/0");

print("Total LOW = {}".format(count))
if count>0:
    input()
