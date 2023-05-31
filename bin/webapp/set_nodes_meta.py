import json

f = open("nodes49-2023.json",)
nodes = json.load(f)
f.close()

f = open("nodes_meta.json",)
meta = json.load(f)
f.close()

print(len(nodes), len(meta))

for n in nodes:
    if n["_id"] != 1:
        for nn in nodes:
            if nn["_id"] == n["parent"]:
                meta[n["eui64"]]["parent"] = nn["eui64"]

with open('nodes_meta.json', 'w') as f:
    json.dump(meta, f)
