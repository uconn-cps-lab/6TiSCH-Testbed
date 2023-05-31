/*
 * This is a dummy simulation to test how many slots are required for each node by a given topology.
 */

var nm      = require('./network_manager');
var settings     = require("./settings.json");
var nodes_meta   = require('./nodes_meta.json');
var slotframe_size = 127;
cloud={data:null,dataCenter:{host:null}};//fix some error msgs
obs_sensor_list={};
obs_nw_perf_list={};
sim = 1;
//settings.scheduler.channels=1;
vpan = {isActive:false};
var topology = require("./mytopology.json");

var seq = [];
/*var dfs = function(id){
  if(topology[id]){
      seq.push(id);
  }
  for(var i in topology){
    if(topology[i]==id){
      dfs(i);
    }
  }
}
dfs(1);*/
var bfs = function(id){
  for(var i in topology){
    if(topology[i]==1&&seq.indexOf(i)==-1){
        seq.push(i);
    }
  }
  for(var j=0;j<seq.length;++j){
    var id=seq[j];
    for(var i in topology){
      if(topology[i]==id&&seq.indexOf(i)==-1){
          seq.push(i);
      }
    }
  }
}
bfs(1);
console.log(seq);

function id2address(id){
  return "2001:db8:1234:ffff::ff:fe00:"+(+id).toString(16);
}

var idx=0;
var timer;
function new_node(socket,id){
	if(nm.sch.isFull){
		console.log("isFull = true");
		console.log("Total number "+idx);
		stat_and_exit();
	}
	socket.sendMessage({
		type:'new_node',
		eui64:'sim'+id.toString(),
		shortAddr:id,
		coordAddr:topology[id]
	});
}
function dao_report(socket,id){
	socket.sendMessage({
		type:'dao_report',
		sender:id2address(id),
		parent:id2address(topology[id]),
		candidate:[],
		lifetime:120
	});
}

function stat_and_exit(){
  console.log("Finished");
  partition={beacon:0,uplink:[],downlink:[]};

  var sender_list=[], receiver_list=[0xffff];
  for(var i=0;i<seq.length;++i){
    sender_list.push(+seq[i]);
  }
  partition.beacon = nm.sch.count_slot(sender_list,receiver_list);
  
  var parents_list=[1];
  var depth=0;
  while(parents_list.length){
    console.log(parents_list);
    var children_list=[];
    for(var i=0;i<parents_list.length;++i){
      var theparent=parents_list[i];
      for(var j=0;j<seq.length;++j){
        var thechild=+seq[j];
        if(theparent==topology[thechild]){
          children_list.push(thechild);
        }
      }
    }
    partition.uplink.push(nm.sch.count_slot(children_list,parents_list));
    partition.downlink.push(nm.sch.count_slot(parents_list,children_list));
    parents_list=children_list;
  }
  console.log(partition);
  process.exit();
}

var JsonSocket = require('json-socket');
var net = require('net');
var server  = new JsonSocket(new net.Socket());
var port = 40001;
var server = net.createServer();
server.listen(port);
server.on('connection', function(socket) {
	socket = new JsonSocket(socket);
	socket.on('message',function(message){
		console.log(message);
    if(message[0]==0x01){
      console.log("ASSOC_STATUS_PAN_AT_CAPA");
      console.log("Total number "+idx);
      stat_and_exit();
    }else{
      //schedule dao
      setTimeout(dao_report.bind(this,socket,seq[idx]),100);
      //proceed to next node
      idx++;
      if(idx<seq.length){
        console.log("schedule next node");
        setTimeout(new_node.bind(this,socket,seq[idx]),200);
      }else{
        console.log("all node added");
        setTimeout(stat_and_exit,200);
      }
    }
	});
	socket.sendMessage({
		type:'set_minimal_config', 
		slot_frame_size:slotframe_size
	});
        setTimeout(new_node.bind(this,socket,seq[idx]),200);
});

var Engine = require('tingodb')({memStore:true});
var db = new Engine.Db('db', {});
function dummycallback(){ return 0; }
nm.setup("localhost",port,db,dummycallback,nodes_meta,settings);
