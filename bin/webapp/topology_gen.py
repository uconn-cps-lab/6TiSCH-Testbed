#!/usr/bin/python3
import sys
import json
lines = sys.stdin.readlines()
topo={}
def process(ln,parent):
	global topo
	indent=lines[ln].count('\t')
	#print("current line =",ln)
	#print("current indent =",indent)
	while ln<len(lines) and lines[ln].count('\t')==indent:
		mac=lines[ln][indent:].rstrip()
		#print("MAC =", mac, "parnet =", parent)
		topo[mac]={'parent':parent,'beacon':1,'uplink':1,'downlink':1};
		ln=ln+1;
		if ln<len(lines) and lines[ln].count('\t')>indent:
			ln=process(ln,mac)
	return ln
process(0,None)
topo['gateway']['uplink']=0
topo['gateway']['downlink']=0
print(json.dumps(topo, indent=4, separators=(',',': ')))
