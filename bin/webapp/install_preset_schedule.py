import os
import json
import time

LINK_OPTION_TX_COAP = 0x21
LINK_OPTION_RX = 0x0A

f = open("schedule_preset.json",)
sch = json.load(f)
f.close()

# add ghost cell
ghost_slots = []
tmp = []
for cell in sch:
    if cell["receiver"] == 1 :
        # print(cell["slot"]["slot_offset"])
        tmp.append(cell["slot"]["slot_offset"])
for s in tmp:
    if s+1 not in tmp:
        ghost_slots.append(s+1)

for slot in ghost_slots:
    os.system("node addSchedule.js {} 15 1 999 1".format(slot))

# current_nodes_meta = [{"_id":1,"address":"2001:db8:1234:ffff::ff:fe00:1","eui64":"gateway","candidate":[],"lifetime":65535,"capacity":65535,"beacon_state":"beacon on","meta":{"gps":[41.8066469,-72.2528862],"power":"USB","type":"LaunchPad"}},{"_id":3,"address":"2001:0db8:1234:ffff:0000:00ff:fe00:0003","parent":1,"candidate":[],"eui64":"00-12-4b-00-13-22-5d-81","capacity":50,"lifetime":103,"beacon_state":"beacon on","meta":{"gps":[41.8066817,-72.2528836],"power":"battery","type":"SensorTag","parent":"gateway"}},{"_id":4,"address":"2001:0db8:1234:ffff:0000:00ff:fe00:0004","parent":1,"candidate":[],"eui64":"00-12-4b-00-0c-65-a0-86","capacity":50,"lifetime":103,"beacon_state":"beacon on","meta":{"gps":[41.8066212,-72.2528847],"power":"battery","type":"SensorTag","parent":"gateway"}},{"_id":5,"address":"2001:0db8:1234:ffff:0000:00ff:fe00:0005","parent":1,"candidate":[],"eui64":"00-12-4b-00-12-04-dc-88","capacity":50,"lifetime":103,"beacon_state":"beacon on","meta":{"gps":[41.8066624,-72.252817],"power":"battery","type":"SensorTag","parent":"gateway"}},{"_id":6,"address":"2001:0db8:1234:ffff:0000:00ff:fe00:0006","parent":1,"candidate":[],"eui64":"00-12-4b-00-12-04-da-e9","capacity":50,"lifetime":103,"beacon_state":"beacon on","meta":{"gps":[41.8066317,-72.2528683],"power":"battery","type":"SensorTag","parent":"gateway"}},{"_id":7,"address":"2001:0db8:1234:ffff:0000:00ff:fe00:0007","parent":5,"candidate":[],"eui64":"00-12-4b-00-0c-46-c7-03","capacity":50,"lifetime":111,"beacon_state":"beacon on","meta":{"gps":[41.8067082,-72.2527412],"power":"battery","type":"SensorTag","parent":"00-12-4b-00-12-04-dc-88"}},{"_id":8,"address":"2001:0db8:1234:ffff:0000:00ff:fe00:0008","parent":3,"candidate":[],"eui64":"00-12-4b-00-12-05-2a-23","capacity":50,"lifetime":112,"beacon_state":"beacon on","meta":{"gps":[41.8067249,-72.2527148],"power":"battery","type":"SensorTag","parent":"00-12-4b-00-13-22-5d-81"}},{"_id":9,"address":"2001:0db8:1234:ffff:0000:00ff:fe00:0009","parent":6,"candidate":[],"eui64":"00-12-4b-00-16-66-2c-07","capacity":50,"lifetime":92,"beacon_state":"beacon on","meta":{"gps":[41.8065496,-72.2527201],"power":"battery","type":"SensorTag","parent":"00-12-4b-00-12-04-da-e9"}},{"_id":10,"address":"2001:0db8:1234:ffff:0000:00ff:fe00:000a","parent":7,"candidate":[],"eui64":"00-12-4b-00-0c-66-d1-03","capacity":50,"lifetime":109,"beacon_state":"beacon on","meta":{"gps":[41.8066803,-72.2526411],"power":"battery","type":"SensorTag","parent":"00-12-4b-00-0c-46-c7-03"}},{"_id":11,"address":"2001:0db8:1234:ffff:0000:00ff:fe00:000b","parent":9,"candidate":[],"eui64":"00-12-4b-00-12-05-27-22","capacity":50,"lifetime":116,"beacon_state":"beacon on","meta":{"gps":[41.8065225,-72.2526929],"power":"battery","type":"SensorTag","parent":"00-12-4b-00-16-66-2c-07"}},{"_id":12,"address":"2001:0db8:1234:ffff:0000:00ff:fe00:000c","parent":10,"candidate":[],"eui64":"00-12-4b-00-0c-64-b3-02","capacity":50,"lifetime":114,"beacon_state":"beacon on","meta":{"gps":[41.8066368,-72.2525928],"power":"battery","type":"SensorTag","parent":"00-12-4b-00-0c-66-d1-03"}}]

# f = open("nodes49-2023.json")
# old_nodes_meta = json.load(f)
# f.close()

# new_id = {1:1}
# for n1 in old_nodes_meta:
#     for n2 in current_nodes_meta:
#         if n2["eui64"]==n1["eui64"]:
#             new_id[n1["_id"]] = n2["_id"]
# print(new_id)
# for cell in sch:
#     slot = cell["slot"]["slot_offset"]
#     ch = cell["slot"]["channel_offset"]
#     if cell["sender"] in new_id and cell["receiver"] in new_id:
#         print("install",cell)
#         sender = new_id[cell["sender"]]
#         receiver = new_id[cell["receiver"]]
#         os.system("node addSchedule.js {} {} {} {} {}".format(slot,ch,sender,receiver, LINK_OPTION_TX_COAP))
#         os.system("node addSchedule.js {} {} {} {} {}".format(slot,ch,receiver,sender,LINK_OPTION_RX))
#         time.sleep(1)