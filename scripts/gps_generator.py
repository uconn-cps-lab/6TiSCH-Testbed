import math
import sys
import json

def gen_circle(LatLon, N, D):
    Lat=LatLon[0];
    Lon=LatLon[1];

    R=6378137;
    dLat=(180/math.pi/R)
    dLon=(180/math.pi/R/math.cos(Lat*math.pi/180))

    Res=[];
    for i in range(N):
        theta=2*math.pi/N*i;
        x = D*math.cos(theta);
        y = D*math.sin(theta);
        nLat = y*dLat + Lat;
        nLon = x*dLon + Lon;
        Res.append([nLat, nLon]);
    return Res;

def gen_line(LatLon0, LatLon1, N):
    Lat0=LatLon0[0];
    Lon0=LatLon0[1];
    Lat1=LatLon1[0];
    Lon1=LatLon1[1];

    dLat=(Lat1-Lat0)/(N-1)
    dLon=(Lon1-Lon0)/(N-1)

    Res=[];
    for i in range(N):
        nLat = i*dLat + Lat0;
        nLon = i*dLon + Lon0;
        Res.append([nLat, nLon]);
    return Res;

nodes_meta=dict();
lines = sys.stdin.readlines();
gps=[];
dev_type=None;
dev_power=None;

line=lines.pop(0);
words=line.split(' ');
z=words[0].split(',');#pixals
x=float(z[0]);y=float(z[1]);
map_pixal=[x,y];
z1=words[1].split(',');
z2=words[2].split(',');
x1=float(z1[0]);y1=float(z1[1]);
x2=float(z2[0]);y2=float(z2[1]);
map_coord=[[x1,y1],[x2,y2]]
def pixal_convert(pixal):
    lat=(map_coord[1][0]-map_coord[0][0])/map_pixal[1]*pixal[1]+map_coord[0][0]
    lon=(map_coord[1][1]-map_coord[0][1])/map_pixal[0]*pixal[0]+map_coord[0][1]
    return [lat,lon]

for line in lines:
    if line[0]=='#':
        continue
    if len(gps)==0:
        #Is instruction
        words=line.split(' ');
        num=int(words[0]);
        dev_type=words[2];
        dev_power=words[3];
        if words[1]=='circle':
            pixal=words[4].split(',')
            pixal=list(map(lambda x:float(x),pixal))
            d=float(words[5]);
            coord=pixal_convert(pixal)
            gps=gen_circle(coord, num, d);
        elif words[1]=='line':
            pixal=words[4].split(',')
            pixal=list(map(lambda x:float(x),pixal))
            coord0=pixal_convert(pixal[0:2])
            coord1=pixal_convert(pixal[2:4])
            gps=gen_line(coord0, coord1, num);
        elif words[1]=='none':
            pixal=words[4].split(',')
            pixal=list(map(lambda x:float(x),pixal))
            coord=pixal_convert(pixal[0:2])
            for i in range(num):
                gps.append([coord[0], coord[1]]);
        else:
            print(line)
            raise BaseException('Error');

    else:
        #Is MAC
        macs=line.rstrip().split(' ')
        nodes_meta[macs[0]]={'gps':gps.pop(0),'type':dev_type,'power':dev_power};
        if(len(macs)>1):
            #nodes_meta[macs[0]]['parent']=macs[1];
            pass
print(json.dumps(nodes_meta, indent=4, sort_keys=True))
