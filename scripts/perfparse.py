import urllib.request
import json
import xlwt

url = 'http://localhost/perf';
#url = 'perf.txt';

if(url.startswith("http")):
	req = urllib.request.Request(url);
	r = urllib.request.urlopen(req).read();
	db = json.loads(r.decode('utf-8'));
else:
	db = json.load(open(url));

book = xlwt.Workbook(encoding="utf-8");
nodedb = {};
for item in db:
	strid=str(item["id"]);
	if not strid in nodedb:
		nodedb[strid]=[];
	nodedb[strid].append(item);

sheet0 = book.add_sheet("stat")
sheet0.write(0, 0, "Node");
sheet0.write(0, 1, "Link-Layer PER");
sheet0.write(0, 2, "Application PER");
sheet0.write(0, 3, "Round Trip Time");
sheet0.write(0, 4, "Num of SyncLost");

node = 0
per_sum = 0
app_per_sum = 0
rtt_sum = 0
num_sync_lost_sum = 0
cnt = 0
for strid in nodedb:
	sheet = book.add_sheet("perf-"+strid)
	sheet.write(0, 0, "Timestamp");
	sheet.write(0, 1, "Link-Layer PER");
	sheet.write(0, 2, "Application PER");
	sheet.write(0, 3, "Round Trip Time");
	sheet.write(0, 4, "Num of SyncLost");
	per_sum0 = 0;
	app_per_sum0 = 0;
	rtt_sum0 = 0;
	cnt0 = 0;
	for item in nodedb[strid]:
		ts = item["ts"]/1000;
		per = item["msg"]["PER"];
		app_per = 0;
		if item["app_per"]["sent"]!=0:
			app_per = item["app_per"]["lost"]/item["app_per"]["sent"]*100;
		rtt = item["rtt"];
		num_sync_lost = item["msg"]["numSyncLost"];

		per_sum0 += per;
		app_per_sum0 += app_per;
		rtt_sum0 += rtt;

		per_sum += per;
		app_per_sum += app_per;
		rtt_sum += rtt;

		cnt0+=1;
		cnt+=1;

		sheet.write(cnt0, 0, ts);
		sheet.write(cnt0, 1, per);
		sheet.write(cnt0, 2, app_per);
		sheet.write(cnt0, 3, rtt);
		sheet.write(cnt0, 4, num_sync_lost);

	num_sync_lost_sum += num_sync_lost
	sheet0.write(node+2, 0, strid);
	sheet0.write(node+2, 1, per_sum0/cnt0);
	sheet0.write(node+2, 2, app_per_sum0/cnt0);
	sheet0.write(node+2, 3, rtt_sum0/cnt0);
	sheet0.write(node+2, 4, num_sync_lost);
	node+=1;

sheet0.write(1, 0, "Network");
sheet0.write(1, 1, per_sum/cnt);
sheet0.write(1, 2, app_per_sum/cnt);
sheet0.write(1, 3, rtt_sum/cnt);
sheet0.write(1, 4, num_sync_lost_sum/node);
book.save("perf.xls");
