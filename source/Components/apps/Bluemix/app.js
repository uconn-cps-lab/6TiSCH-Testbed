/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


//================================================================================
// MODULES AND GLOBALS
//================================================================================
var express 		= require('express'),
    bodyParser = require('body-parser'),
		os 				= require('os'),
		fs 				= require('fs'),
		path 			= require('path'),
		http			= require('http'),
		appFront 		= express(),
		coap 			= require('coap'),
		wss 			= require('ws').Server, 
		ws 				= new wss({port: 5000}),
		dgram 			= require('dgram'),
		settings 		= require("./settings.json"),
		ntfyCount		= 0,
		timeStart 		= Math.floor(Date.now()/1000),
		session			= (Math.floor((Math.random() * 10000) + 1)).toString(),
		log 			= {},
    gwMgmt          = require('./gwMgmt'),
    coap_manage     = require('./coap_manage'),
    network_manager     = require('./network_manager'),
		cloud 		= require("./cloud.json");

var global_data = {};
var newSensorData = 0;
//================================================================================
// EXCEPTION HANDLER AND ERROR LOGGING
//================================================================================

function logError(error) {

	if (!log.hasOwnProperty(session)){
		log[session] = [];
	} 

	log[session].push(error);

	fs.writeFile("./log.json", JSON.stringify(log,null,4), function(err) {
	    if(err) {
	        console.log("couldn't write errorLog.json file");
	    }
	}); 
  console.log("error: "+error);
  console.trace();
}

try {
	log = require("./log.json");
} catch (e) {
	//errorLog.json does not exist, start with empty error log.
}

//catch any uncaught exceptions and log them
process.on('uncaughtException', function (err) {
	//log error
	console.log("uncaughtException: "+err);
	var timeSinceStart = Math.floor(Date.now()/1000) - timeStart;
	var error = {"type":"uncaughtException",timeSinceStart:timeSinceStart,"msg":JSON.stringify(err.message)};
	logError(error);
	//restart app
//	process.exit(1);
})

//================================================================================
// LISTENERS
//================================================================================

var httpFrontendServer,
		udpBackendServer,
		Engine, 
    coapObserver,
		db;

//HTTP Front-end listener
httpFrontendServer = appFront.listen(80);

//UDP Back-end listener
udpBackendServer = dgram.createSocket('udp6');
udpBackendServer.bind(settings.conninfo.udpPort);

coapObserver=[];

console.log("");
console.log("**********");
console.log("SmartNet Webapp Backend");
console.log("**********");
console.log("");
console.log("Session ID: "+session);
console.log("");
console.log("UDP  Back-end listening on port: " + settings.conninfo.udpPort);
console.log("");

//================================================================================
// DATABASE
//================================================================================

if (true) {
  if (!fs.existsSync('db')) {
    fs.mkdirSync('db');
  }
  Engine = require('tingodb')({memStore:true});
  db = new Engine.Db('db', {});

} else {
  Engine = require('mongodb');
  db = new Engine.Db('db', new Engine.Server('localhost', 27017));
}

//================================================================================
// EXPRESS
//================================================================================

//serve front-end w/cache 
var oneDay = 86400000;
appFront.use(bodyParser.urlencoded({extended:true}));
appFront.use(express.static(__dirname + '/iot', { maxAge: oneDay }));

//================================================================================
// WEBSOCKET
//================================================================================
ws.broadcast = function(data) {
	for(var i in this.clients) {
    try{
      this.clients[i].send(data);
    }catch(err){
      console.log("Client send fail");
    }
	}
};
ws.on('connection', function(ws) {
	console.log("Websocket client connected");
	ws.on('close', function() {
		console.log("Websocket connection closed");
	});
});


//================================================================================
// HELPER FUNCTIONS
//================================================================================
function getRandomInt(min, max) {
	return Math.floor(Math.random() * (max - min + 1)) + min;
}

function getMACAddress() {
	var mac="00:00:00:00:00:00";
	var devs = fs.readdirSync('/sys/class/net/');
	devs.forEach(function(dev) {
		var fn = path.join('/sys/class/net', dev, 'address');
		//if(dev.substr(0, 4) == 'eth0' && fs.existsSync(fn)) {
		if(dev.substr(0, 6) == 'enp0s4' && fs.existsSync(fn)) {
			mac = fs.readFileSync(fn).toString().trim();
		}
	});
	return mac;
}
function getIPAddress() {
	var ip;
	var ifaces = os.networkInterfaces();
	if ('lpn0' in ifaces){ 
		ifaces['lpn0'].some(function(details){
			if (details.family=='IPv6') {
				ip = details.address;
				if (ip.match(/^[23]/i) != null){
					return true;	// global unicast IPv6 address
				};
				if (ip.match(/^fd/i) != null){
					return true; 	// unique local IPv6 address
				};
			};
			return false;
		});
	};
	return ip;
}

function setConfig(gwUpdate) {

	var proxy;
	if (gwUpdate.hasOwnProperty('proxy')) {
		proxy = gwUpdate['proxy'];
	} else {
		proxy = '';  
	}

	if (gwUpdate.hasOwnProperty('keys')) {
		if (gwUpdate['keys'].length === 0 || !gwUpdate['keys'].trim()) {
			keys = [];
		} else {
			var keyStrings = gwUpdate['keys'].split(',');
			var keys = keyStrings.map(function (x) {
				return parseInt(x);
			});
		}
	}

	if (!gwUpdate.hasOwnProperty('port')) {
		gwUpdate['port'] = 4730;
	} 

	settings['appOnER'] = gwUpdate['appOnER'];
	settings['nwkinfo']['netname'] = gwUpdate['netname'];
	settings['nwkinfo']['gps'] = gwUpdate['gps'];
	settings['nwkinfo']['mac'] = getMACAddress();
	settings['conninfo']['api_ver'] = parseInt(gwUpdate['api_ver']);
	settings['conninfo']['httpServer'] = gwUpdate['server'];
	settings['conninfo']['httpPort'] = parseInt(gwUpdate['port']);
	settings['conninfo']['keys'] = keys;
	settings['conninfo']['proxy'] = proxy;

	//replace settings.json for persistient storage
	fs.writeFileSync("settings.json", JSON.stringify(settings,null,'\t'), "utf8");

}

//================================================================================
// FRONT-END API
//================================================================================

//get ALL networks
//processes GET to /nwk
appFront.get('/nwk', function (req, res) {
	//temporary fix for memStore:true
	res.type('application/json');
	return res.json([{"_id":"00","info":{"netname":"IoT Gateway"}}])

	//query networks collection
	db.collection('networks', function (err, collection) {
		if (!err) {
			collection.find().toArray(function (err, items) {
				if (!err) {
					//respond with JSON data
					res.statusCode = 200;
					res.type('application/json');
					res.json(items);
					console.log("Sent networks to " + req.connection.remoteAddress);
				} else {
					logError(err);
					res.statusCode = 400;
					return res.send('Error 400: get unsuccessful');
				}
			});
		} else {
			logError(err);
			res.statusCode = 400;
			return res.send('Error 400: get unsuccessful');
		}
	});

});


//get ALL authorized nodes for network <nui>
//processes GET to /nwk/<nui>/node
appFront.get('/nwk/:nui/node', function (req, res) {
	var nui = req.params.nui;

	//query network <nui> collection
	db.collection("nodes", function (err, collection) {
		if (!err) {
			var query;

			if (nui != 'any') {
				query = {auth: 1, network: nui};
			} else {
				query = {auth: 1};
			}

			collection.find(query).toArray(function (err, items) {
				if (!err) {
					//respond with JSON data
					res.statusCode = 200;
					res.type('application/json');
					res.json(items);
					console.log("Sent all connected nodes to " + req.connection.remoteAddress);
				} else {
					logError(err);
					res.statusCode = 400;
					return res.send('Error 400: get unsuccessful');
				}
			});

		} else {
			logError(err);
			res.statusCode = 400;
			return res.send('Error 400: get unsuccessful');
		}
	});

});

appFront.get('/nwk/any/count', function (req, res) {
	db.collection("nodes", function(err, collection) {
		if (err) {
      logError(err);
			res.statusCode = 400;
			return res.send('Error 400: get unsuccessful');
		}

		collection.find({ auth: 1 }).count(function(err, c) {
			res.statusCode = 200;
			res.type('application/json');
			res.json({ count: c });
		});
	});
});

//get ALL unauthorized (newly joined) nodes for network <nui>
//processes GET to /nwk/<nui>/node
appFront.get('/nwk/:nui/node/new', function (req, res) {
	var nui = req.params.nui;

	//query network <nui> collection
	db.collection("nodes", function (err, collection) {
		if (!err) {
			collection.find({auth: 0, network: nui}).toArray(function (err, items) {
				if (!err) {
					//respond with JSON data
					res.statusCode = 200;
					res.type('application/json');
					res.json(items);
					console.log("Sent nodes of network " + nui + " to " + req.connection.remoteAddress);
				} else {
					logError(err);
					res.statusCode = 400;
					return res.send('Error 400: get unsuccessful');
				}
			});
		} else {
			logError(err);
			res.statusCode = 400;
			return res.send('Error 400: get unsuccessful');
		}
	});

});


var firstConnect = 0;
//connect new node 
//processes PUT to /nwk/<nui>/node/<eui>
appFront.put('/nwk/:nui/node/:eui/auth/:auth', function (req, res) {
	var nui = req.params.nui;
	var eui = req.params.eui;
	var auth = parseInt(req.params.auth);

	//query network <nui> collection
	db.collection("nodes", function (err, collection) {
		if (!err) {

			collection.update({_id: eui, network: nui}, {$set: {auth: auth}});

      collection.find({_id: eui, network: nui}).toArray(function (err, items) {
        if (!err) {
          if(items[0]!=null){
            var node=items[0];
            if(auth==1){
              coapObserveStart(node);
              console.log("Sending Observe Request to node: "+node._id);
            }else{
              coapObserveStop(node);
            }
            ws.broadcast(JSON.stringify(node));
            console.log("Connected node " + eui + " on network " + nui);
          }else{
            console.log("Node: "+node._id+"undefined");
          }
        }
      });

			//respond (no body)
			res.statusCode = 200;
			res.send();
		}
	});

});




//edit node name
//processes PUT to /group/<groupName>
appFront.put('/nwk/:nui/node/:eui/name/:name', function (req, res) {
	var nui = req.params.nui;
	var eui = req.params.eui;
	var name = req.params.name;

	//query network <nui> collection
	db.collection("nodes", function (err, collection) {
		if (!err) {

			collection.update({_id: eui, network: nui}, {$set: {name: name}});

			//respond (no body)
			res.statusCode = 200;
			res.send();
			console.log("Changed name of node " + eui + " to " + name);
		}
	});
});


//network deletion
//processes DELETE to /nwk/<nui>
appFront.delete('/nwk/:nui', function (req, res) {
	var nui = req.params.nui;

	//TODO

	//respond (no body)
	res.statusCode = 200;
	res.send();
	console.log("Network " + nui + " removed");
});


//node deletion
//processes DELETE to /nwk/<nui>/node/<eui>
appFront.delete('/node/:eui', function (req, res) {
	var eui = req.params.eui;



	//delete node <eui>
	db.collection("nodes", function (err, collection) {
		if (!err) {
			if (eui =='all') {
				collection.remove({});
			} else {			
				collection.remove({_id: eui}, true);
			}
		}
		else {
			logError(err);
			res.statusCode = 400;
			return res.send('Error 400: delete unsuccessful');
		}
	});

	//respond (no body)
	res.statusCode = 200;
	res.send();
	console.log("Node " + eui + " removed from Network " + nui);
});


// FRONT-END API - GROUPS API
//================================================================================

//Add nodes to group
//processes PUT to /node/group/<groupID> with array of nodes in body
appFront.put('/node/group/:groupName', function (req, res) {
	var groupName = req.params.groupName;
	var euis = req.body.euis;
	//TODO: add error control
	//query network <nui> collection
	db.collection("nodes", function (err, collection) {
		if (!err) {

			collection.update({'_id': {$in: euis}}, {$set: {group: groupName}}, {multi: true}, function (err) {

				db.collection("nodes", function (err, collectionNetwork) {
					if (!err) {
						collectionNetwork.find({group: groupName}).count(function (e, cnt) {

							db.collection('groups', function (err, groupCollection) {
								if (!err) {
									groupCollection.update({name: groupName}, {$set: {count: cnt}});
								}
							});

							//respond (no body)
							res.statusCode = 200;
							res.send();
							console.log("Changed group for nodes " + euis + " to group " + groupName);
						});
					}

				});
			});
		}

	});

});


//add new group
//processes PUT to /group/<groupName>
appFront.put('/group/:groupName', function (req, res) {
	var groupName = req.params.groupName;

	//query network <nui> collection
	db.collection('groups', function (err, collection) {
		if (!err) {

			collection.update({name: groupName}, {$set: {count: 0, description: 'GroupDescription'}}, {upsert: true});

			//respond (no body)
			res.statusCode = 200;
			res.send();
			console.log("Added group " + groupName);
		}
	});
});

//rename group
//processes PUT to /group/<current group name>/rename/<new group name> 
appFront.put('/group/:groupName/rename/:newName', function (req, res) {
	var groupName = req.params.groupName;
	var newName = req.params.newName;

	//query network <nui> collection
	db.collection('groups', function(error, groups) {
		if (!error) {
			groups.update({ name: groupName }, { $set: { name: newName } }, { upsert: false });
			groups.remove({ name: groupName });

			db.collection("nodes", function(error, nodes) {
				if (!error) {
					nodes.update({ group: groupName }, { $set: { group: newName } }, { upsert: false });
				}
			});

			//respond (no body)
			res.statusCode = 200;
			res.send();
			console.log("Renamed group " + groupName + ' to ' + newName);
		}
	});
});


//change group description
//processes PUT to /group/<group>/desc with body as description 
appFront.put('/group/:group/desc', function (req, res) {
	var group = req.params.group;
	var desc = req.body.desc;

	//query network <nui> collection
	db.collection('groups', function(error, groups) {
		if (!error) {
			groups.update({ name: group }, { $set: { description: desc } }, { upsert: false });

			//respond (no body)
			res.statusCode = 200;
			res.send();
			console.log("Changed description of group " + group + ' to ' + desc);
		}
	});
});

//delete group
//processes DELETE to /nwk/<nui>/group/<groupName>
appFront.delete('/group/:groupName', function (req, res) {
	var groupName = req.params.groupName;

	//query network <nui> collection
	db.collection("groups", function (err, groupCollection) {
		if (!err) {

			groupCollection.remove({name: groupName}, {justOne: false});

			db.collection("nodes", function (err, networkCollection) {
				if (!err) {
					networkCollection.update({group: groupName}, {$set: {group: 0}}, {multi: true});
				}
			});


			//respond (no body)
			res.statusCode = 200;
			res.send();
			console.log("deleted group " + groupName);
		}
	});
});


//get group list
//processes GET to /group
appFront.get('/group', function (req, res) {

	//query network <nui> collection
	db.collection('groups', function (err, collection) {
		if (!err) {
			collection.find().toArray(function (err, items) {
				if (!err) {
					//respond with JSON data
					res.statusCode = 200;
					res.type('application/json');
					res.json(items);
					console.log("Sent groups");
				} else {
					logError(err);
					res.statusCode = 400;
					return res.send('Error 400: get groups unsuccessful');
				}
			});
		}
		else {
			logError(err);
			res.statusCode = 400;
			return res.send('Error 400: get groups unsuccessful');
		}
	});
});

//get nodes for specific group
//processes GET to /nwk/<nui>/group/<groupName>
appFront.get('/group/:groupName', function (req, res) {
	var groupName = req.params.groupName;

	//query network <nui> collection
	db.collection("nodes", function (err, collection) {
		if (!err) {
			collection.find({group: groupName}).toArray(function (err, items) {
				if (!err) {
					//respond with JSON data
					res.statusCode = 200;
					res.type('application/json');
					res.json(items);
					console.log("Sent nodes of group " + groupName);
				} else {
					logError(err);
					res.statusCode = 400;
					return res.send('Error 400: get unsuccessful');
				}
			});
		}
		else {
			logError(err);
			res.statusCode = 400;
			return res.send('Error 400: get unsuccessful');
		}
	});
});


// FRONT-END API - CONFIGURATION API
//================================================================================

//get conection info 
//processes GET to /config/conn
appFront.get('/config/gw', function (req, res) {
	res.statusCode = 200;
	res.type('application/json');
	res.send(settings);
});

//configure conection info 
//processes POST to /config/conn
//if cloud server selected, also send create Network to cloud backend
appFront.post('/config/gw', function (req, res) {
	var gwUpdate = req.body;
	//required fields check 
	if (!gwUpdate.hasOwnProperty('netname') ||
		!gwUpdate.hasOwnProperty('gps') ||
		!gwUpdate.hasOwnProperty('appOnER') ||
		!gwUpdate.hasOwnProperty('server') ||
		!gwUpdate.hasOwnProperty('api_ver') )
	{
		console.log("Error 400: <conninfo> syntax incorrect.");
		res.statusCode = 400;
		return res.end();
	}

	setConfig(gwUpdate);

	//respond success
	res.statusCode = 200;
	res.send();
	console.log("Gateway settings updated to:");
	console.log(settings);
	console.log("");

});

// FRONT-END APT - ACTION API
//================================================================================

//set action
//processes POST to /node/<eui>/action/<action> with body as data
appFront.post('/node/:eui/action/:action', function(req, res) {
	var eui = req.params.eui;
	var action = req.params.action;	
	var body = req.body;
	var ntfy;

	ntfyCount++;
	var tm = Math.floor(Date.now()/1000) - timeStart;
	var ac = parseInt(action, 16);

	var ntfyQuery = {
		"ntfy.act":{
			"id": ntfyCount,
			"tm": tm,
			"ac": ac
		}
	};

	var ntfyCollection = db.collection('notifications');

	ntfyCollection.update({eui:eui}, { $set: ntfyQuery },{ upsert: true }, function(err, result) {
		if (err) {
			logError(err);
		}
	});

	res.statusCode = 200;
	console.log("Added action notification " +action+" for node: " +eui);
	res.send();

});

//================================================================================
// BACK-END - HELPER FUNCTIONS
//================================================================================

function getNtfy(eui, callback) {
	db.collection('notifications', function(err, collection) {
		if (!err) {
			collection.find( {eui:eui}).toArray(function(err, items) {
				if (!err) {
					return( callback(0,items[0].ntfy) );
				} else {
					logError(err);
					return( callback(-1) );
				}
			});
		}
		else {
			logError(err);
			return( callback(-1) );
		}
	});
}

function newNetwork(networkinfo, callback) {
	//required fields check 
	if (!networkinfo.hasOwnProperty('netname') || !isNaN(networkinfo.netname) ||
		!networkinfo.hasOwnProperty('type') || isNaN(networkinfo.type) ||
		!networkinfo.hasOwnProperty('ver') || isNaN(networkinfo.ver) ||
		!networkinfo.hasOwnProperty('mla') || isNaN(networkinfo.mla) ||
		!networkinfo.hasOwnProperty('panid') || isNaN(networkinfo.panid) ||
		!networkinfo.hasOwnProperty('msa') || isNaN(networkinfo.msa) ||
		!networkinfo.hasOwnProperty('prefix') || !isNaN(networkinfo.prefix) ||
		!networkinfo.hasOwnProperty('gua') || !isNaN(networkinfo.gua) ||
		!networkinfo.hasOwnProperty('mac') || !isNaN(networkinfo.mac) ||
		!networkinfo.hasOwnProperty('key') || isNaN(networkinfo.key))
	{
		return( callback(-2) );
	}

	//setup structure for network data storage 
	var networksStruct = {
		_id: networkinfo.mac,
		timestamp: Date.now(),
		info: networkinfo
	};

	//add networks collection (if none) and insert network (will overwrite if network already exsits)
	db.createCollection("networks", function (err, collection) {
		if (!err) {
			collection.update({_id: networksStruct._id}, networksStruct, {upsert: true}, function (err, result) {
				if (err) {
					logError(err);
					return( callback(-1) );
				}
				return( callback(0) );
			});
		}
		else {
			logError(err);
			return( callback(-1) );
		}
	});

}

function newNode(nui,eui,nodeinfo, callback) {
	//required fields check 
	if (!nodeinfo.hasOwnProperty('api_ver') || isNaN(nodeinfo.api_ver) ||
		!nodeinfo.hasOwnProperty('hw_ver') || isNaN(nodeinfo.hw_ver) ||
		!nodeinfo.hasOwnProperty('sw_ver') || isNaN(nodeinfo.sw_ver) ||
		!nodeinfo.hasOwnProperty('mla') || isNaN(nodeinfo.mla) ||
		!nodeinfo.hasOwnProperty('msa') || isNaN(nodeinfo.msa) ||
		!nodeinfo.hasOwnProperty('prod') || !isNaN(nodeinfo.prod) ||
		!nodeinfo.hasOwnProperty('mfr') || !isNaN(nodeinfo.mfr) ||
		!nodeinfo.hasOwnProperty('dev') || !isNaN(nodeinfo.dev) ||
	//!nodeinfo.hasOwnProperty('logi_type') || isNaN(nodeinfo.logi_type) ||
	//!nodeinfo.hasOwnProperty('pwr_type') || isNaN(nodeinfo.pwr_type) ||
	!nodeinfo.hasOwnProperty('rtg_up_pnt') || isNaN(nodeinfo.rtg_up_pnt[0].addr))
	{
		return( callback(-2) );
	}

	//setup structure for node data storage 
	var nodeStruct = {
		_id: parseInt(eui),
		network: nui,
		name: String(eui),
		timestamp: Date.now(),
		group: 0,
	auth: 0, //if auth=0, the node has not been allowed to connect via the front-end node management interface
	info: nodeinfo,
	status: {},
	sensor: {},
	log: {}
	};

	//add node to network collection and insert nodeinfo (will overwrite if node already exsits)
	db.collection("nodes", function (err, collection) {
		if (!err) {
			collection.update({_id: eui}, nodeStruct, {upsert: true}, function (err, result) {
				if (err) {
					logError(err);
					return( callback(-1) );
				}
			});

		//broadcast data to front-end websocket clients
		ws.broadcast(JSON.stringify(nodeStruct));
		return( callback(0) );
		}
		else {
			logError(err);
			return( callback(-1) );
		}
	});

}

function updateNode(nui,eui,updateType,update, callback) {
	var nodeStruct;

	//select network collection and update <updateType>
	db.collection("nodes", function (err, collection) {
		if (!err) {
			var updateStruct = {};
			updateStruct[updateType] = update;
			updateStruct['timestamp'] = Date.now();
			collection.update({_id: eui}, {$set: updateStruct}, {upsert: true}, function (err, result) {
				if (err) {
					logError(err);
					return( callback(-1) );
				}
				//console.log("update: "+result);
			});

			//broadcast data to front-end websocket clients
			updateStruct['_id'] = eui;
			updateStruct['network'] = nui;
			updateStruct['auth'] = 1;
			//console.log("broadcast: "+updateStruct);

			ws.broadcast(JSON.stringify(updateStruct));
			return( callback(0) );
		}
		else {
			logError(err);
			return( callback(-1) );
		}
	});

}

//================================================================================
// CoAP BACK-END
//================================================================================

var coapNodes = [];
udpBackendServer.on('message', function (message, remote) {
console.log("GOT NNI: "+message);
	try {
		var nodeinfo = JSON.parse(message);
	}
	catch (e) {
		console.log("ERROR: NNI parse error: "+e);
		return;
	}

	var nui = getMACAddress();
	var eui = nodeinfo.mla;
	delete nodeinfo['nui'];
	nodeinfo['protocol'] = 'coap';
	var msa = Number(nodeinfo['msa']);
	nodeinfo['coap'] = {"srcAddr":'2001:db8:1234:ffff:0000:00ff:fe00:'+msa.toString(16)};

	console.log(nui + "/" +eui+ " port: " + remote.port);

	newNode(nui,eui,nodeinfo, function (error) {
		if (error == 0) {
			console.log("CoAP: New node init - "+nui+"/"+eui);
		} else if (error == -1) {
			console.log("CoAP: New node init - error adding node to DB");
		} else {
			console.log("CoAP: New node init - <nodeinfo> syntax incorrect.");
		}
	});
});

function parseCoapGetSensors(data, node) {
		
  try {
    var sensorData = JSON.parse(data);
  } catch (e) {
    logError(e);
    return;
  }
  updateNode(node.network,node._id,'sensor',sensorData, function (error) {
    if (error == 0) {
      console.log("CoAP: Update node - "+node.info.coap.srcAddr+"/sensors");
    } else if (error == -1) {
      console.log("CoAP: Update node - error updating DB");
    } else {
      console.log("CoAP: Update node - must specify <updateType>");
    }
  });
};

function coapObserveStop(node) {
  for(var i in coapObserver){
    if(coapObserver[i].node==node._id){
      coapObserver[i].observer.close();
      console.log("CoAP Observe stopped: "+node._id);
    }
  }
}

function coapObserveStart(node) {
	console.log("CoAP: GET request to: "+node._id);
	var req = coap.request({
        hostname: node.info.coap.srcAddr,
        method: 'GET',
        confirmable: false,
        observe: true,
        pathname: '/sensors',
        agent: new coap.Agent({ type: 'udp6'})
	});

	req.on('response', function(res) {
    console.log("CoAP Observe Response");
    coapObserver.push({observer:res,node:node._id});
    res.setEncoding('utf8');
    res.on('data', function (data) {
      console.log("CoAP Observe Data: "+data);
      parseCoapGetSensors(data, node);
    });

	})
	req.on('error', function(err) {
		console.log("CoAP Observe Error: "+err);
	})

	req.end();

};




//CoAP PUT request 
//processes PUT to /coap/:eui/:resource/:data
appFront.put('/coap/:eui/:resource/:data', function (req, res) {
	var eui = req.params.eui;
	var resource = req.params.resource;
	var data = req.params.data;
	console.log('CoAP: PUT to '+eui+'/'+resource+'/'+data);
	db.collection("nodes", function (err, collection) {
		if (!err) {
			var query = {auth:1};
			//query['info.protocol'] = 'coap';
			query['_id'] = eui;
      collection.find(query).toArray(function (err, items) {
        if (!err) {
          node=items[0];
          if(node!=null){
            var req = coap.request({
                  hostname: node.info.coap.srcAddr,
                  method: 'PUT',
                  confirmable: false,
                  pathname: '/'+resource,
                  agent: new coap.Agent({ type: 'udp6'})
            });
            req.write(data,"utf8");
            req.end();

          }else{
            console.log("Unable to find node: "+eui);
          }
        } else {
          logError(err);
        }
      });				
		} else {
			logError(err);
		}
	});
});

//---------------------
//Tao's code
//---------------------

network_manager.setup("localhost",40000,db);
var col=db.collection('nm');
var obs_list={};

function obs_parse(id,data){
  obs_list[id].freshness=Date.now();
  var obj_data = {};
  var idx=0;
  obj_data.tamb=data.readInt16BE(idx)/100;idx+=2;
  obj_data.rhum=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.lux=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.press=data.readUInt32BE(idx)/100;idx+=4;
  obj_data.gyrox=data.readInt16BE(idx)/100;idx+=2;
  obj_data.gyroy=data.readInt16BE(idx)/100;idx+=2;
  obj_data.gyroz=data.readInt16BE(idx)/100;idx+=2;
  obj_data.accelx=data.readInt16BE(idx)/100;idx+=2;
  obj_data.accely=data.readInt16BE(idx)/100;idx+=2;
  obj_data.accelz=data.readInt16BE(idx)/100;idx+=2;
  obj_data.led=data.readUInt8(idx);idx+=1;
  obj_data.channel=data.readUInt8(idx);idx+=1;
  obj_data.bat=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.eh=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.eh1=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.cc2650_active=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.cc2650_sleep=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.rf_tx=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.rf_rx=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.ssm_active=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.ssm_sleep=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.gpsen_active=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.gpsen_sleep=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.msp432_active=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.msp432_sleep=data.readUInt16BE(idx)/100;idx+=2;
  obj_data.others=data.readUInt16BE(idx)/100;idx+=2;
  var msg={_id:id,sensors:obj_data};
  console.log("observer data: "+id);
  ws.broadcast(JSON.stringify(msg));
  global_data = {type:"sensor_type_0", gateway_0:cloud.data, sensor_0: {_id:id, data:obj_data}};
  newSensorData = 1;
  sendDataToCloud();
}

function obs_start(id){
  console.log("observer: "+id+" starts");
  col.find({_id:id}).toArray(function(err,items){
    if(err||items[0]==null||items[0].address==null){
      console.log("Node address not found: "+id);
      if(obs_list[id]!=null)
        delete obs_list[id];
    }else{
      if(obs_list[id]!=null){
        console.log("observer : "+id+" already running");
        return;
      }
      obs_list[id]={freshness:Date.now()};
      var coap_client = coap.request({
            hostname: items[0].address,
            method: 'GET',
            confirmable: false,
            observe: true,
            pathname: '/sensors',
            agent: new coap.Agent({ type: 'udp6'})
      });
      obs_list[id].coap_client=coap_client;

      coap_client.on('response', function(res) {
        obs_list[id].observer=res;
        res.on('data', function (data) {
          obs_parse(id, data);
        });
      });

      coap_client.on('error', function(err) { })
      coap_client.end();
    }
  });
}

setInterval(function(){
  const obs_timeout = 60*1000;
  for(var id in obs_list){
    if(Date.now()-obs_list[id].freshness>obs_timeout){
      if(obs_list[id].observer!=null)
        obs_list[id].observer.close();
      delete obs_list[id];
      console.log("observer: "+id+" timeout");
      var msg={_id:id};
      ws.broadcast(JSON.stringify(msg));

      obs_start(id);
    }
  }
},5000);

appFront.get('/nodes/count', function (req, res) {
  col.count({lifetime:{$gt:0}},function(err,count){
    if(err){
      res.statusCode = 400;
	res.type('application/json');
      res.send("400");
    }else{
      res.statusCode = 200;
      res.send(count.toString());
    }
  });
});

appFront.get('/nodes/:id', function (req, res) {
	var id = req.params.id;
  col.find({_id:id}).toArray(function(err,items){
    if(err||items[0]==null){
      res.statusCode = 400;
      res.send("400");
    }else{
      res.statusCode = 200;
      res.type('application/json');
      res.json(items[0]);
      if(obs_list[id]==null){
        obs_start(id);
      }
    }
  });
});

appFront.put('/nodes/:id/:res/:data', function (req, res) {
	var id = req.params.id;
	var resource = req.params.res;
	var data = req.params.data;
  console.log("coap put: "+id+"/"+resource+"/"+data);
  col.find({_id:id}).toArray(function(err,items){
    if(err||items[0]==null){
      res.statusCode = 400;
      res.send("400");
    }else{
      var coap_client = coap.request({
            hostname: items[0].address,
            method: 'PUT',
            confirmable: false,
            observe: false,
            pathname: '/'+resource,
            agent: new coap.Agent({ type: 'udp6'})
      });
      coap_client.on('response',function(){
        console.log("coap put: response!")
        res.statusCode = 200;
        res.send();
      });
      coap_client.on('error',function(){
        console.log("coap put: error!")
      });
      coap_client.write(data,'ascii');
      coap_client.end();
    }
  });
});

appFront.get('/nodes/:id/:res', function (req, res) {
	var id = req.params.id;
	var resource = req.params.res;
  console.log("coap get: "+id+"/"+resource);
  col.find({_id:id}).toArray(function(err,items){
    if(err||items[0]==null){
      res.statusCode = 400;
      res.send("400");
    }else{
      var coap_client = coap.request({
            hostname: items[0].address,
            method: 'GET',
            confirmable: false,
            observe: false,
            pathname: '/'+resource,
            agent: new coap.Agent({ type: 'udp6'})
      });
      coap_client.on('response',function(msg){
        console.log("coap get: response!"+msg.payload);
        res.statusCode = 200;
        res.send(msg.payload);
      });
      coap_client.on('error',function(){
        console.log("coap get: error!")
        res.statusCode = 400;
        res.send("400");
      });
      coap_client.end();
    }
  });
});

appFront.get('/nodes', function (req, res) {
  col.find({lifetime:{$gt:0}})
    .toArray(function(err,items){
    if(err){
      res.statusCode = 400;
      res.send("400");
    }else{
      res.statusCode = 200;
      res.type('application/json');
      res.json(items);
    }
  });
})



function isEmpty(obj) 
{
	return (Object.keys(obj).length === 0 && obj.constructor === Object);
}

function sendDataToCloud()
{
  if(cloud!=null&&
    cloud.host!=null&&
    cloud.port!=null&&
    cloud.path!=null&&
    cloud.data!=null&&
    cloud.data.name!="FIXME"
  ){
	  //global_data = cloud.data;
  if (newSensorData && !isEmpty(global_data))  //FMDEBUG
  {  
	var options =
	{
		hostname: cloud.host,
		path: cloud.path,
        port: cloud.port,
		method: 'POST',
		headers: 
		{
				'Content-Type': 'application/json',
		}
	};
	
	var req = http.request(options, function(res) 
	{
		res.on('data', function (body) {});
	});
	
	req.on('error', function(e) { });
	// write data to request body
	//req.write(JSON.stringify(cloud.data));
	req.write(JSON.stringify(global_data));
	newSensorData = 0;
	console.log("Post data:"+cloud.path);

		req.end(); 
	  }
  }
}

//setInterval(sendDataToCloud,5000);
