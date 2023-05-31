import urllib.request
import json
import time

url = 'http://localhost/power';

f = open("powerparse.txt", "w");
while True:
	if(url.startswith("http")):
		req = urllib.request.Request(url);
		r = urllib.request.urlopen(req).read();
		db = json.loads(r.decode('utf-8'));
	else:
		db = json.load(open(url));

	power_total = 0
	for id in db:
		item = db[id]
		tx = item["tx"]
		rx = item["rx"]
		total = tx+rx
		print("ID"+id+":"+"TX("+str(tx)+"),RX("+str(rx)+"), total("+str(total)+")")
		power_total +=total
	print("Total: "+str(power_total))
	f.write(r.decode('utf-8'));
	f.write("\n");
	f.flush();
	time.sleep(10);

