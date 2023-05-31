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
var express = require('express'),
   bodyParser = require('body-parser'),
   // os = require('os'),
   fs = require('fs'),
   path = require('path'),
   coap = require('coap'),
   wss = require('ws').Server,
   dgram = require('dgram'),
   network_manager = require('./network_manager'),
   powerstat = require("./powerstat");

//================================================================================
//Globals:
multigw = require('./multigw');
cloud = require("./cloud.json");
id2eui64 = require('./network_manager').id2eui64;
obs_sensor_list = {};
obs_nw_perf_list = {};

//================================================================================
//Locals:
var SelfReloadJSON = require('self-reload-json');

var ws = new wss({ port: 5000 }),
   appFront = express(),
   ntfyCount = 0,
   timeStart = Math.floor(Date.now() / 1000),
   session = (Math.floor((Math.random() * 10000) + 1)).toString(),
   log = {};

var nodes_meta = new SelfReloadJSON('./nodes_meta.json');
var settings = new SelfReloadJSON('./settings.json');

var udpBackendServer,
   Engine,
   coapObserver,
   db;

var coapTiming =
{
   ackTimeout: 60,
   ackRandomFactor: 10,
   maxRetransmit: 0,
   maxLatency: 30,
   piggybackReplyMs: 10
};

// var oneDay = 86400000;
// var coapNodes = [];
var col;
// var perf;
var latency;
var nwDataSet0;
var nwDataSet1;
var nwDataSet2;
var dbUrl;
usingMongoDb = false;
col_update = nm_col_update_func;

var last_packet_created_asn = {}
var last_packet_a2a_latency = {}

var HCT_TLV_TYPE_TEMP = 0x11;
var HCT_TLV_TYPE_HUM = 0x12;
var HCT_TLV_TYPE_LIGHT = 0x13;
var HCT_TLV_TYPE_PRE = 0x14;
var HCT_TLV_TYPE_MOT = 0x15;
var HCT_TLV_TYPE_LED = 0x16;
var HCT_TLV_TYPE_CHANNEL = 0x17;
var HCT_TLV_TYPE_BAT = 0x18;
var HCT_TLV_TYPE_EH = 0x19;
var HCT_TLV_TYPE_CC2650_POWER = 0x1A;
var HCT_TLV_TYPE_MSP432_POWER = 0x1B;
var HCT_TLV_TYPE_SENSOR_POWER = 0x1C;
var HCT_TLV_TYPE_OTHER_POWER = 0x1D;
var HCT_TLV_TYPE_SEQUENCE = 0x1E;
var HCT_TLV_TYPE_ASN_STAMP = 0x1F;
var HCT_TLV_TYPE_LAST_SENT_ASN = 0x20;
var HCT_TLV_TYPE_COAP_DATA_TYPE = 0x21;
var HCT_TLV_TYPE_RTT = 0x22
var HCT_TLV_TYPE_SLOT_OFFSET = 0x23
var pingIndex = 0;

var sensors_start = false;
var TRAFFIC_RATE = 1;

var ideal_m2m_latency = {}

// var firstConnect = 0;
var TESTING_LAYER = 2;
var NVM_STATE_INACTIVE = 0;
var NVM_STATE_UPDATING = 1;
var NVM_STATE_COMMITING = 2;
var NVM_STATE_UPDATED = 3;
var NVM_STATE_PULLING = 4;
var NVM_STATE_PULLING_DONE = 5;

var MAX_NVMPARAM_ARRAY_SIZE = 300;

var NVM_node_array = [];
var nvm_node = null;
var nvm_gw = null;
var gw_address = null;
var nvm_update_current_idx = 0;
var nvm_search_count_down = 0;
var nvmNodeList = null;
var nodeChildList = {};
var NVM_node_pull_index = [];
var restrictedEui = null;

const httpPort = 8080;

var nodeLocationArray = [];
var fixedNodeList = require("./fixed_nodes.json");
const DIST_MIN = 0.8;  //meters, minimal distance between the localization beacons
const MIN_RSSI_FOR_LOCALIZATION = -95;
const MAX_RSSI_FOR_LOCALIZATION = -10;
const MIN_NUM_ANCHOR_NODES = 3;
const MAX_NUM_ANCHOR_NODES = 5;
const anchorNodes =
   [
      [1, 2, 3],
      [1, 2, 4],
      [1, 3, 4],
      [2, 3, 4],
      [1, 2, 5],
      [1, 3, 5],
      [1, 4, 5],
      [2, 3, 5],
      [2, 4, 5],
      [3, 4, 5]
   ];
var ref_lat, ref_longi;
// var testRssiReport =
//    [
//       { eui: '4199c53f', rssi: -58 },
//       { eui: '4199ce03', rssi: -66 },
//       { eui: '4199be34', rssi: -68 }
//    ];
// var testNodeEui = "00-12-4b-00-18-99-c8-71";

//********************************************************************************
//Initial code starts
//********************************************************************************
try {
   log = require("./log.json");
}
catch (e) {
   //errorLog.json does not exist, start with empty error log.
}

var preset_topo = require('./nodes49-2023.json');

function old_id(eui64) {
  if (eui64 == "gateway") return 1
  for (let node of preset_topo) {
    if (node.eui64 == eui64) {
      return node._id
    }
  }
  return -1
}

const source_nodes = [15,29,37,22,25,45,3,4,9,20,32,47]

//================================================================================
// LISTENERS
//================================================================================
//HTTP Front-end listener
appFront.listen(httpPort);

//UDP Back-end listener
udpBackendServer = dgram.createSocket('udp6');
udpBackendServer.bind(settings.conninfo.udpPort);

coapObserver = [];

console.log("");
console.log("**********");
console.log("Webapp Backend");
console.log("**********");
console.log("");

multigw.setup({
   web_broadcast: web_broadcast, http_get: http_get, http_put: http_put, settings: settings, httpPort: httpPort,
   db_find_nwk_nui_nodes: db_find_nwk_nui_nodes, db_find_groups: db_find_groups,
   db_node_count: db_node_count, db_node_find: db_node_find, get_node_id_callback: get_node_id_callback,
   get_nodes_callback: get_nodes_callback, init_callback: initCode1, node_joined_another_pan_handler: node_joined_another_pan_handler
});

multigw.contact_vpan_peers();

//======================================================
//End of init
//======================================================
function initCode1() {
   coap.updateTiming(coapTiming);
   //================================================================================
   // DATABASE
   //================================================================================
   if (settings.mongoDbUrl) {
      dbUrl = settings.mongoDbUrl;
   }

   if (!dbUrl) {
      createTingoDb();
      initCode2();
   }
   else {
      Engine = require('mongodb').MongoClient;
      Engine.connect(dbUrl, function (err, dbs) {
         if (err) {
            createTingoDb();
         }
         else {
            console.log("Store data in MongoDB: " + dbUrl);
            var dbName = (cloud.data.name) ? cloud.data.name : 'db';
            db = dbs.db(dbName);
            usingMongoDb = true;
         }

         initCode2();
      });
   }
}

function timeStamp() {
   var now = new Date();
   var date = [now.getMonth() + 1, now.getDate(), now.getFullYear()];
   var time = [now.getHours(), now.getMinutes(), now.getSeconds()];
   for (var i = 1; i < 3; i++) {
      if (time[i] < 10) {
         time[i] = "0" + time[i];
      }
   }
   return date.join("/") + " " + time.join(":");
}

//======================================================
function createTingoDb() {
   console.log("Store data in Tingo DB");
   if (!fs.existsSync('db')) {
      fs.mkdirSync('db');
   }
   Engine = require('tingodb')({ memStore: false });
   db = new Engine.Db('./db', {});
   usingMongoDb = false;
}
//======================================================
function initCode2() {

   // db.collection('nm').remove({}, function (err, removed) { });
   db.collection('nm').remove({}, function () { });
   col = db.collection('nm');
   // perf = db.collection('perf');
   nwDataSet0 = db.collection('nwDataSet0');
   nwDataSet1 = db.collection('nwDataSet1');
   nwDataSet2 = db.collection('bleBeacons')
   latency = db.collection('latency');

   //================================================================================
   // EXPRESS
   //================================================================================

   //serve front-end w/cache
   appFront.use(bodyParser.urlencoded({ extended: true }));
   appFront.use(express.static(__dirname + '/iot', { maxAge: 1 }));

   //================================================================================
   // WEBSOCKET
   //================================================================================
   ws.broadcast = webbroadcast
   //================================================================================
   // CoAP BACK-END
   //================================================================================
   network_manager.setup("127.0.0.1", 40000, db, obs_start, nodes_meta, settings);
   //================================================================================
   //Start the period timers
   //================================================================================
   setInterval(observerTimeoutHandler, 5000);
   // setInterval(network_manager.sendDataToCloud,120000);;
   // setInterval(pingHandler, 10000);

   setInterval(() => {
      col.compactCollection(() => { })
      // console.log("compact collection")
   }, 5 * 60 * 1000)

   //================================================================================
   //NVM initialization
   //================================================================================
   //   nvm_init();
   //================================================================================
   //Localization initialization
   //================================================================================
   initLocalization();
   //================================================================================
   //Set the handlers for the events
   //================================================================================
   process.on('uncaughtException', uncaugtErrorHandler)
   ws.on('connection', websocketConnectionHandler);

   //Set the back end server RX data handler
   udpBackendServer.on('message', udpBackendServer_RxHandler);

   //cors allow
   appFront.all('/*', function (req, res, next) {
      res.header("Access-Control-Allow-Origin", "*");
      res.header("Access-Control-Allow-Headers", "X-Requested-With");
      next();
   });

   //Set the web server command handlers:
   appFront.get('/nwk', appFrontGetNwkHandler);
   appFront.get('/nwk/:nui/node', appFrontGetNwkNuiNodeHandler);
   appFront.get('/nwk/any/count', appFrontGetNwkAnyCountHandler);
   appFront.get('/nwk/:nui/node/new', appFrontGetNwkNuiNodeNewHandler);
   appFront.put('/nwk/:nui/node/:eui/auth/:auth', appFrontPutNwkNuiNodeEuiAuthAuthHandler);
   appFront.put('/nwk/:nui/node/:eui/name/:name', appFrontPutNwkNuiNodeEuiNameNameHandler);
   appFront.delete('/nwk/:nui', appFrontDeleteNwkNuiHandler);
   appFront.delete('/node/:eui', appFrontDeleteNodeEuiHandler);
   appFront.put('/node/group/:groupName', appFrontPutNodeGroupGroupenameHandler);
   appFront.put('/group/:groupName', appFrontPutGroupGroupenameHandler);
   appFront.put('/group/:groupName/rename/:newName', appFrontPutGroupenameRenameNewnameHandler);
   appFront.put('/group/:group/desc', appFrontPutGroupGroupDescHandler);
   appFront.delete('/group/:groupName', appFrontDeleteGroupGroupnameHandler);
   appFront.get('/group', appFrontGetGroupHandler);
   appFront.get('/group/:groupName', appFrontGetGroupGroupnameHandler);
   appFront.get('/config/gw', appFrontGetConfigGwHandler);
   appFront.post('/config/gw', appFrontPostConfigGwHandler);
   appFront.post('/node/:eui/action/:action', appFrontPostNodeEuiActionActionHandler);
   appFront.put('/coap/:eui/:resource/:data', appFrontPutCoapEuiResourceDataHandler);
   appFront.get('/nodes/count', appFrontGetNodesCountHandler);
   appFront.get('/schedule', appFrontGetScheduleHandler);
   appFront.get('/schedule_old', appFrontGetScheduleOldHandler);
   appFront.get('/schedule/:id', appFrontGetScheduleIdHandler);
   // appFront.put('/schedule/:id', appFrontPutScheduleIdHandler);
   appFront.put('/schedule/rx/:sender/:receiver', appFrontPutAddRxHandler);

   appFront.put('/topolock/:flag', appFrontPutTopoLockHandler);

   appFront.get('/sensors_start/:flag', appFrontGetSensorStartInitHandler);

   appFront.get('/nodes/:id', appFrontGetNodesIdHandler);
   appFront.delete('/nodes/:id', appFrontDeleteNodesIdHandler);
   appFront.put('/nodes/:id/:res/:data', appFrontPutNodeIdResDataHandler);
   appFront.get('/nodes/:id/:res', appFrontGetNodeIdResDataHandler);
   appFront.get('/nodes', appFrontGetNodesHandler)
   appFront.get('/nwDataSet0', appFrontGetNwDataSet0Handler);
   appFront.get('/nwDataSet0/:id', appFrontGetNwDataSet0IdHandler);
   appFront.get('/nwDataSet1', appFrontGetNwDataSet1Handler);
   appFront.get('/nwDataSet1/:id', appFrontGetNwDataSet1IdHandler);
   appFront.get('/bleBeacons', appFrontGetNwDataSet2Handler);
   appFront.get('/bleBeacons/:id', appFrontGetNwDataSet2IdHandler);
   appFront.get('/latency', appFrontGetLatencyHandler);
   appFront.get('/latency/:id', appFrontGetLatencyIdHandler);
}
//********************************************************************************
//Initial code ends
//********************************************************************************
function nm_col_update_func(col, querr, val, upsertrec, terminateOnError) {
   if (!usingMongoDb) {
      col.update(querr, val, upsertrec);
   }
   else {
      col.update(querr, val, upsertrec, function (err, res) {
         if (err || res.result.ok != 1) {
            console.log("#######dB update error:" + JSON.stringify(err));
            if (terminateOnError) {
               process.exit(0);
            }
         }
      });
   }
}

//Hooks to the multi-GW module begin
function node_joined_another_pan_handler(panAddr) {
   network_manager.node_departure_process(panAddr);
   col.remove({ _id: panAddr });

}

function get_node_id_callback(items) {
   nodes_meta.forceUpdate();
   items[0].meta = nodes_meta[items[0].eui64];
   if (items[0].meta == null) items[0].meta = {};
   if (obs_sensor_list[items[0]._id] == null || obs_sensor_list[items[0]._id].deleted) {
      obs_start(items[0]._id);
   }
}

function get_nodes_callback(items) {
   nodes_meta.forceUpdate();
   for (var i = 0; i < items.length; ++i) {
      items[i].meta = nodes_meta[items[i].eui64];
      if (items[i].meta == null) items[i].meta = {};
   }
}

function http_get(url, handler) {
   appFront.get(url, handler);
}

function http_put(url, handler) {
   appFront.put(url, handler);
}

function web_broadcast(data) {
   ws.broadcast(data);
}

function db_find_nwk_nui_nodes(cond, callback) {
   var nui = cond;
   db.collection("nodes", function (err, collection) {
      if (!err) {
         var query;

         if (nui != 'any') {
            query = { auth: 1, network: nui };
         }
         else {
            query = { auth: 1 };
         }

         collection.find(query).toArray(callback);

      }
      else {
         callback(err, null);
      }
   });
}
//================================================================================
function db_find_groups(cond, callback) {
   db.collection('groups', function (err, collection) {
      if (!err) {
         collection.find().toArray(callback);
      }
      else {
         callback(err, null);
      }
   });
}
//================================================================================
function db_node_count(cond, callback) {
   if (col) {
      col.count(cond, callback);
   }
   else {
      callback("Col null", 0);
   }
}
//================================================================================
function db_node_find(cond, callback) {
   if (col) {
      col.find(cond).toArray(callback);
   }
   else {
      callback("Col null", null);
   }
}
//Hooks to the multi-GW module end
//================================================================================
//Event handlers starts
//================================================================================
//observer timeout handler
function observerTimeoutHandler() {
   const sens_obs_timeout = 60 * 1000;
   const nw_perf_obs_timeout = 180 * 1000;

   for (var id in obs_sensor_list) {
      if (!obs_sensor_list[id].deleted && (Date.now() - obs_sensor_list[id].freshness > sens_obs_timeout)) {
         obs_sensor_stop(+id);
         console.log("sensor observer for node " + id + " timeout");
         var msg = { _id: +id };
         multigw.vpan_adjust_params(msg);
         ws.broadcast(JSON.stringify(msg));
         multigw.broadcastFrontEndDataToServers(JSON.stringify(msg));
         multigw.vpan_restore_params(msg);
      }
   }

   for (var id in obs_nw_perf_list) {
      if (!obs_nw_perf_list[id].deleted && (Date.now() - obs_nw_perf_list[id].freshness > nw_perf_obs_timeout)) {
         obs_nw_perf_stop(+id);
         console.log("network performance observer for node " + id + " timeout");
         var msg = { _id: +id };
         multigw.vpan_adjust_params(msg);
         ws.broadcast(JSON.stringify(msg));
         multigw.broadcastFrontEndDataToServers(JSON.stringify(msg));
         multigw.vpan_restore_params(msg);
      }
   }
}

//ping handler
function pingHandler() {
   col.find({ _id: { $ne: multigw.gw_id }, lifetime: { $gt: 0 } }).toArray(function (err, items)   //jira77gw
   {
      if (err) {
      }
      else if (items.length > 0) {
         if (pingIndex >= items.length) {
            pingIndex = 0;
         }
         pingNode(+items[pingIndex]._id);
         pingIndex++;
      }
   });
}

//Uncaught error event handler
function uncaugtErrorHandler(err) {
   //log error
   console.log("uncaughtException: " + err);
   console.error(err.stack);

   var timeSinceStart = Math.floor(Date.now() / 1000) - timeStart;
   var error = { "type": "uncaughtException", timeSinceStart: timeSinceStart, "msg": JSON.stringify(err.message) };
   logError(error);
   //restart app
   //process.exit(1);
}

//New web client connection event handler
function websocketConnectionHandler(ws) {
   console.log("Websocket client connected");
   multigw.inform_vpan_peers_frontend_connected();
   ws.on('close', function () {
      console.log("Websocket connection closed");
      multigw.inform_vpan_peers_frontend_disconnected();
   });
}
//================================================================================
//Web front end command handlers
//================================================================================
//processes GET to /nwk
function appFrontGetNwkHandler(req, res) {

   //temporary fix for memStore:true
   res.type('application/json');
   return res.json([{ "_id": "00", "info": { "netname": "IoT Gateway" } }])

   //query networks collection
   // db.collection('networks', function (err, collection) {
   //    if (!err) {
   //       collection.find().toArray(function (err, items) {
   //          if (!err) {
   //             //respond with JSON data
   //             res.statusCode = 200;
   //             res.type('application/json');
   //             res.json(items);
   //             console.log("Sent networks to " + req.connection.remoteAddress);
   //          }
   //          else {
   //             logError(err);
   //             res.statusCode = 400;
   //             return res.send('Error 400: get unsuccessful');
   //          }
   //       });
   //    }
   //    else {
   //       logError(err);
   //       res.statusCode = 400;
   //       return res.send('Error 400: get unsuccessful');
   //    }
   // });
}

function appFrontGetSensorStartInitHandler(req, res) {
   // console.log(req.params.finished)
   if (req.params.flag != 1) {
      sensors_start = false
   }
   else {
      sensors_start = true
   }
   console.log("set sensors_start to", sensors_start)
   return res.send("set sensors_start")
}


//=======================================================
// lock nodes to specific parents
function appFrontPutTopoLockHandler(req, res) {
   network_manager.set_topo_lock(req.params.flag)
   console.log("topo lock set to", req.params.flag)
   return res.send("topo lock set to " + req.params.flag)
}

//=======================================================
// add rx link
function appFrontPutAddRxHandler(req, res) {
   network_manager.add_rx(req.params.sender, req.params.receiver)
   console.log("add rx", req.params.sender, req.params.receiver)
   return res.send("add rx")
}

//=======================================================
//get ALL authorized nodes for network <nui>
//processes GET to /nwk/<nui>/node
function appFrontGetNwkNuiNodeHandler(req, res) {
   //console.log('appFrontGetNwkNuiNodeHandler() enter');
   var peer_arg = { 'peeridx': -1, 'req': req, 'res': res, 'nui': req.params.nui };
   multigw.getNextPeerNwkNuiNode(peer_arg, function (err, items) {
      if (!err) {
         //query network <nui> collection
         db_find_nwk_nui_nodes(peer_arg.nui, function (err, localitems) {
            if (!err) {
               localitems = localitems.concat(items);
               //respond with JSON data
               peer_arg.res.statusCode = 200;
               peer_arg.res.type('application/json');
               peer_arg.res.json(localitems);
               //console.log("Sent all connected nodes to " + peer_arg.req.connection.remoteAddress);
            }
            else {
               logError(err);
               peer_arg.res.statusCode = 400;
               return peer_arg.res.send('Error 400: get unsuccessful');
            }
         });
      }
      else {
         logError(err);
         peer_arg.res.statusCode = 400;
         return peer_arg.res.send('Error 400: get unsuccessful');
      }
   });
}

//==============================================
function appFrontGetNwkAnyCountHandler(req, res) {
   db.collection("nodes", function (err, collection) {
      if (err) {
         logError(err);
         res.statusCode = 400;
         return res.send('Error 400: get unsuccessful');
      }

      collection.find({ auth: 1 }).count(function (err, c) {
         res.statusCode = 200;
         res.type('application/json');
         res.json({ count: c });
      });
   });
}
//get ALL unauthorized (newly joined) nodes for network <nui>
//processes GET to /nwk/<nui>/node
function appFrontGetNwkNuiNodeNewHandler(req, res) {
   var nui = req.params.nui;

   //query network <nui> collection
   db.collection("nodes", function (err, collection) {
      if (!err) {
         collection.find({ auth: 0, network: nui }).toArray(function (err, items) {
            if (!err) {
               //respond with JSON data
               res.statusCode = 200;
               res.type('application/json');
               res.json(items);
               console.log("Sent nodes of network " + nui + " to " + req.connection.remoteAddress);
            }
            else {
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
}
//connect new node
//processes PUT to /nwk/<nui>/node/<eui>
function appFrontPutNwkNuiNodeEuiAuthAuthHandler(req, res) {
   var nui = req.params.nui;
   var eui = req.params.eui;
   var auth = parseInt(req.params.auth);

   //query network <nui> collection
   db.collection("nodes", function (err, collection) {
      if (!err) {

         collection.update({ _id: eui, network: nui }, { $set: { auth: auth } });

         collection.find({ _id: eui, network: nui }).toArray(function (err, items) {
            if (!err) {
               if (items[0] != null) {
                  var node = items[0];
                  if (auth == 1) {
                     coapObserveStart(node);
                     console.log("Sending Observe Request to node: " + node._id);
                  }
                  else {
                     coapObserveStop(node);
                  }

                  multigw.vpan_adjust_params(node);
                  ws.broadcast(JSON.stringify(node));
                  multigw.broadcastFrontEndDataToServers(JSON.stringify(node));
                  multigw.vpan_restore_params(node);
                  console.log("Connected node " + eui + " on network " + nui);
               }
               else {
                  console.log("Node: " + node._id + "undefined");
               }
            }
         });

         //respond (no body)
         res.statusCode = 200;
         res.send();
      }
   });
}
//edit node name
//processes PUT to /group/<groupName>
function appFrontPutNwkNuiNodeEuiNameNameHandler(req, res) {
   var nui = req.params.nui;
   var eui = req.params.eui;
   var name = req.params.name;

   //query network <nui> collection
   db.collection("nodes", function (err, collection) {
      if (!err) {

         collection.update({ _id: eui, network: nui }, { $set: { name: name } });

         //respond (no body)
         res.statusCode = 200;
         res.send();
         console.log("Changed name of node " + eui + " to " + name);
      }
   });
}
//network deletion
//processes DELETE to /nwk/<nui>
function appFrontDeleteNwkNuiHandler(req, res) {
   var nui = req.params.nui;

   //TODO

   //respond (no body)
   res.statusCode = 200;
   res.send();
   console.log("Network " + nui + " removed");
}
//node deletion
//processes DELETE to /nwk/<nui>/node/<eui>
function appFrontDeleteNodeEuiHandler(req, res) {
   var eui = req.params.eui;
   var nui = req.params.nui;
   //delete node <eui>
   db.collection("nodes", function (err, collection) {
      if (!err) {
         if (eui == 'all') {
            collection.remove({});
         }
         else {
            collection.remove({ _id: eui }, true);
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
}
//Add nodes to group
//processes PUT to /node/group/<groupID> with array of nodes in body
function appFrontPutNodeGroupGroupenameHandler(req, res) {
   var groupName = req.params.groupName;
   var euis = req.body.euis;
   //TODO: add error control
   //query network <nui> collection
   db.collection("nodes", function (err, collection) {
      if (!err) {
         collection.update({ '_id': { $in: euis } }, { $set: { group: groupName } }, { multi: true }, function () {
            db.collection("nodes", function (err, collectionNetwork) {
               if (!err) {
                  collectionNetwork.find({ group: groupName }).count(function (e, cnt) {
                     db.collection('groups', function (err, groupCollection) {
                        if (!err) {
                           groupCollection.update({ name: groupName }, { $set: { count: cnt } });
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
}
//add new group
//processes PUT to /group/<groupName>
function appFrontPutGroupGroupenameHandler(req, res) {
   var groupName = req.params.groupName;

   //query network <nui> collection
   db.collection('groups', function (err, collection) {
      if (!err) {
         collection.update({ name: groupName }, { $set: { count: 0, description: 'GroupDescription' } }, { upsert: true });

         //respond (no body)
         res.statusCode = 200;
         res.send();
         console.log("Added group " + groupName);
      }
   });
}
//rename group
//processes PUT to /group/<current group name>/rename/<new group name>
function appFrontPutGroupenameRenameNewnameHandler(req, res) {
   var groupName = req.params.groupName;
   var newName = req.params.newName;

   //query network <nui> collection
   db.collection('groups', function (error, groups) {
      if (!error) {
         groups.update({ name: groupName }, { $set: { name: newName } }, { upsert: false });
         groups.remove({ name: groupName });

         db.collection("nodes", function (error, nodes) {
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
}
//change group description
//processes PUT to /group/<group>/desc with body as description
function appFrontPutGroupGroupDescHandler(req, res) {
   var group = req.params.group;
   var desc = req.body.desc;

   //query network <nui> collection
   db.collection('groups', function (error, groups) {
      if (!error) {
         groups.update({ name: group }, { $set: { description: desc } }, { upsert: false });

         //respond (no body)
         res.statusCode = 200;
         res.send();
         console.log("Changed description of group " + group + ' to ' + desc);
      }
   });
}
//delete group
//processes DELETE to /nwk/<nui>/group/<groupName>
function appFrontDeleteGroupGroupnameHandler(req, res) {
   var groupName = req.params.groupName;

   //query network <nui> collection
   db.collection("groups", function (err, groupCollection) {
      if (!err) {
         groupCollection.remove({ name: groupName }, { justOne: false });

         db.collection("nodes", function (err, networkCollection) {
            if (!err) {
               networkCollection.update({ group: groupName }, { $set: { group: 0 } }, { multi: true });
            }
         });


         //respond (no body)
         res.statusCode = 200;
         res.send();
         console.log("deleted group " + groupName);
      }
   });
}
//==================================================
//get group list
//processes GET to /group
function appFrontGetGroupHandler(req, res) {
   //console.log('appFrontGetGroupHandler() enter');
   var gpg_arg = { 'peeridx': -1, 'req': req, 'res': res };
   multigw.getNextPeerGroup(gpg_arg, function (err, items) {
      if (!err) {
         db_find_groups(null, function (err, localitems) {
            if (!err) {
               localitems = localitems.concat(items);
               //respond with JSON data
               gpg_arg.res.statusCode = 200;
               gpg_arg.res.type('application/json');
               gpg_arg.res.json(localitems);
               //console.log("Sent groups");
            }
            else {
               logError(err);
               gpg_arg.res.statusCode = 400;
               return gpg_arg.res.send('Error 400: get groups unsuccessful');
            }
         });
      }
      else {
         logError(err);
         gpg_arg.res.statusCode = 400;
         return gpg_arg.res.send('Error 400: get groups unsuccessful');
      }
   });
}

//===============================
//get nodes for specific group
//processes GET to /nwk/<nui>/group/<groupName>
function appFrontGetGroupGroupnameHandler(req, res) {
   var groupName = req.params.groupName;

   //query network <nui> collection
   db.collection("nodes", function (err, collection) {
      if (!err) {
         collection.find({ group: groupName }).toArray(function (err, items) {
            if (!err) {
               //respond with JSON data
               res.statusCode = 200;
               res.type('application/json');
               res.json(items);
               console.log("Sent nodes of group " + groupName);
            }
            else {
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
}
//processes GET to /config/conn
function appFrontGetConfigGwHandler(req, res) {
   var tsettings = JSON.parse(JSON.stringify(settings));
   if (tsettings.vpan) {
      tsettings.vpan.password = null;
   }
   res.statusCode = 200;
   res.type('application/json');
   res.send(tsettings);
}

//processes POST to /config/conn
//if cloud server selected, also send create Network to cloud backend
function appFrontPostConfigGwHandler(req, res) {
   var gwUpdate = req.body;
   //required fields check
   if (!gwUpdate.hasOwnProperty('netname') ||
      !gwUpdate.hasOwnProperty('gps') ||
      !gwUpdate.hasOwnProperty('appOnER') ||
      !gwUpdate.hasOwnProperty('server') ||
      !gwUpdate.hasOwnProperty('api_ver')) {
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

}
//processes POST to /node/<eui>/action/<action> with body as data
function appFrontPostNodeEuiActionActionHandler(req, res) {
   var eui = req.params.eui;
   var action = req.params.action;
   // var body = req.body;
   // var ntfy;

   ntfyCount++;
   var tm = Math.floor(Date.now() / 1000) - timeStart;
   var ac = parseInt(action, 16);

   var ntfyQuery = {
      "ntfy.act":
      {
         "id": ntfyCount,
         "tm": tm,
         "ac": ac
      }
   };

   var ntfyCollection = db.collection('notifications');

   ntfyCollection.update({ eui: eui }, { $set: ntfyQuery }, { upsert: true }, function (err) {
      if (err) {
         logError(err);
      }
   });

   res.statusCode = 200;
   console.log("Added action notification " + action + " for node: " + eui);
   res.send();

}

function udpBackendServer_RxHandler(message, remote) {
   console.log("GOT NNI: " + message);
   try {
      var nodeinfo = JSON.parse(message);
   }
   catch (e) {
      console.log("ERROR: NNI parse error: " + e);
      return;
   }

   var nui = getMACAddress();
   var eui = nodeinfo.mla;
   delete nodeinfo['nui'];
   nodeinfo['protocol'] = 'coap';
   var msa = Number(nodeinfo['msa']);
   nodeinfo['coap'] = { "srcAddr": '2001:db8:1234:ffff:0000:00ff:fe00:' + msa.toString(16) };

   console.log(nui + "/" + eui + " port: " + remote.port);

   newNode(nui, eui, nodeinfo, function (error) {
      if (error == 0) {
         console.log("CoAP: New node init - " + nui + "/" + eui);
      }
      else if (error == -1) {
         console.log("CoAP: New node init - error adding node to DB");
      }
      else {
         console.log("CoAP: New node init - <nodeinfo> syntax incorrect.");
      }
   });
}

//processes PUT to /coap/:eui/:resource/:data
function appFrontPutCoapEuiResourceDataHandler(req, res) {
   var eui = req.params.eui;
   var resource = req.params.resource;
   var data = req.params.data;
   console.log('CoAP: PUT to ' + eui + '/' + resource + '/' + data);
   db.collection("nodes", function (err, collection) {
      if (!err) {
         var query = { auth: 1 };
         //query['info.protocol'] = 'coap';
         query['_id'] = eui;
         collection.find(query).toArray(function (err, items) {
            if (!err) {
               var node = items[0];
               if (node != null) {
                  var req = coap.request(
                     {
                        hostname: node.info.coap.srcAddr,
                        method: 'PUT',
                        confirmable: false,
                        pathname: '/' + resource,
                        agent: new coap.Agent({ type: 'udp6' })
                     });
                  req.write(data, "utf8");

                  req.on('error', function () {
                     console.log("appFrontPutCoapEuiResourceDataHandler: error!");
                     req.freeCoapClientMemory();
                  });
                  req.on('timeout', function () {
                     console.log("appFrontPutCoapEuiResourceDataHandler: timeout!");
                     req.freeCoapClientMemory();
                  });
                  req.end();
               }
               else {
                  console.log("Unable to find node: " + eui);
               }
            }
            else {
               logError(err);
            }
         });
      }
      else {
         logError(err);
      }
   });
}
//============================
function appFrontGetNodesCountHandler(req, res) {
   //console.log('appFrontGetNodesCountHandler() enter');
   var gnpnc_arg = { 'peeridx': -1, 'req': req, 'res': res };
   multigw.getNextPeerNodeCount(gnpnc_arg, function (err, count) {
      if (err) {
         gnpnc_arg.res.statusCode = 400;
         gnpnc_arg.res.type('application/json');
         gnpnc_arg.res.send("400");
      }
      else {
         db_node_count({ lifetime: { $gt: 0 } }, function (err, localcount) {
            if (!err) {
               localcount += count;
               gnpnc_arg.res.statusCode = 200;
               gnpnc_arg.res.send((localcount - 1).toString());
            }
            else {
               gnpnc_arg.res.statusCode = 400;
               gnpnc_arg.res.type('application/json');
               gnpnc_arg.res.send("400");
            }
         });
      }
   });
}

//========================================
function appFrontGetScheduleHandler(req, res) {
   var schedule = network_manager.get_schedule();
   res.statusCode = 200;
   res.send({ flag: 1, msg: "success", data: schedule });
}

function appFrontGetScheduleOldHandler(req, res) {
   var schedule = network_manager.get_schedule_old(0);
   res.statusCode = 200;
   res.send(schedule);
}

function appFrontGetScheduleIdHandler(req, res) {
   // var id = +req.params.id;
   var schedule = network_manager.get_schedule();
   if (schedule == null) {
      res.statusCode = 400;
      res.send("400");
   }
   else {
      res.statusCode = 200;
      res.send(schedule);
   }
}


//========================================================

function appFrontGetNodesIdHandler(req, res) {
   var id = +req.params.id;
   var org_res = res;

   if (id > 0) {
      var gw = multigw.vpanAddrToGw(id);
      if (gw == multigw.vpanNodeAddressBase()) {
         id = multigw.vpanAddrTopanAddr(id);
         col.find({ _id: id }).toArray(function (err, items) {
            if (err || items[0] == null) {
               org_res.statusCode = 400;
               org_res.send("400");
            }
            else {
               org_res.statusCode = 200;
               org_res.type('application/json');
               nodes_meta.forceUpdate();
               items[0].meta = nodes_meta[items[0].eui64];
               if (items[0].meta == null) items[0].meta = {};
               multigw.vpan_adjust_params(items[0]);
               org_res.json(items[0]);
               if (obs_sensor_list[id] == null || obs_sensor_list[id].deleted) {
                  obs_start(id);
               }
            }
         });
      }
      else {
         multigw.getNodeIdFromPeerGateway(id, gw, function (err, item) {
            if (!err) {
               //console.log('appFrontGetNodesIdHandler():'+'['+i+']='+JSON.stringify(item));
               org_res.statusCode = 200;
               org_res.type('application/json');
               org_res.json(item);
            }
            else {
               console.log(`problem with request (14):` + err);
               org_res.statusCode = 400;
               org_res.send("400");
            }
         });
      }
   }
   else {
      org_res.statusCode = 400;
      org_res.send("400");
   }
}

//======================================================
function appFrontDeleteNodesIdHandlerHandler(aobj, callback) {
   var req = aobj.req;
   var res = aobj.res;
   var id = +req.params.id;
   if (!multigw.isGateway(id))   //jira77gw
   {
      col.remove({ _id: id });
      obs_sensor_stop(id);
      obs_nw_perf_stop(id);
   }
   res.statusCode = 200;
   res.send("200");
   callback();
}

function appFrontDeleteNodesIdHandler(req, res) {
   network_manager.sequencer_add({ req: req, res: res }, appFrontDeleteNodesIdHandlerHandler);
}
//-------------------------------------------------------------
function appFrontPutNodeIdResDataHandler(req, res) {
   var id = +req.params.id;
   var resource = req.params.res;
   var data = req.params.data;
   console.log("coap put: " + id + "/" + resource + "/" + data);
   col.find({ _id: id }).toArray(function (err, items) {
      if (err || items[0] == null) {
         res.statusCode = 400;
         res.send("400");
      }
      else {
         var coap_client = coap.request(
            {
               hostname: items[0].address,
               method: 'PUT',
               confirmable: false,
               observe: false,
               pathname: '/' + resource,
               agent: new coap.Agent({ type: 'udp6' })
            });
         coap_client.on('response', function () {
            console.log("coap put: response!");
         });
         coap_client.on('error', function (err) {
            console.log("coap put: error!", err);
            coap_client.freeCoapClientMemory();
         });
         coap_client.on('timeout', function () {
            console.log("coap put: timeout!");
            coap_client.freeCoapClientMemory();
         });
         coap_client.write(data, 'ascii');
         coap_client.end();
         res.statusCode = 200;
         res.send();
      }
   });
}

function appFrontGetNodeIdResDataHandler(req, res) {
   var id = +req.params.id;
   var resource = req.params.res;
   console.log("coap get: " + id + "/" + resource);
   col.find({ _id: id }).toArray(function (err, items) {
      if (err || items[0] == null) {
         res.statusCode = 400;
         res.send("400");
      }
      else {
         var coap_client = coap.request(
            {
               hostname: items[0].address,
               method: 'GET',
               confirmable: false,
               observe: false,
               pathname: '/' + resource,
               agent: new coap.Agent({ type: 'udp6' })
            });
         coap_client.on('response', function (msg) {
            console.log("coap get: response!" + msg.payload);
            res.statusCode = 200;
            res.send(msg.payload);
         });
         coap_client.on('error', function () {
            console.log("coap get: error!")
            res.statusCode = 400;
            res.send("400");
            coap_client.freeCoapClientMemory();
         });
         coap_client.on('timeout', function () {
            console.log("coap get: timeout!")
            res.statusCode = 400;
            res.send("400");
            coap_client.freeCoapClientMemory();
         });
         coap_client.end();
      }
   });
}
//=========================================
function appFrontGetNodesHandler(req, res) {
   //console.log('appFrontGetNodesHandler() enter');
   var gpn_arg = { 'peeridx': -1, 'req': req, 'res': res };
   multigw.getNextPeerNodes(gpn_arg, function (err, items) {
      if (err) {
         gpn_arg.res.statusCode = 400;
         gpn_arg.res.send("400");
         console.log(err);
      }
      else {
         db_node_find({}, function (err, localitems) {
            if (err) {
               gpn_arg.res.statusCode = 400;
               gpn_arg.res.send("400");
               console.log(err);
            }
            else {
               multigw.vpanNodesListPurge(localitems);
               get_nodes_callback(localitems);
               for (var i = 0; i < localitems.length; ++i) {
                  multigw.vpan_adjust_params(localitems[i]);
               }
               localitems = localitems.concat(items);
               gpn_arg.res.statusCode = 200;
               gpn_arg.res.type('application/json');
               gpn_arg.res.json(localitems);
            }
         });
      }
   });
}

//==============================================
function appFrontGetNwDataSet0Handler(req, res) {
   var now = new Date().getTime();
   nwDataSet0.find({ ts: { $gt: (now - 2 * 24 * 60 * 60 * 1000) } }).toArray(function (err, items) {
      if (err) {
         res.statusCode = 400;
         res.send("400");
      }
      else {
         res.statusCode = 200;
         res.type('application/json');
         res.json(items);
      }
   });
}

function appFrontGetNwDataSet0IdHandler(req, res) {
   var id = +req.params.id;
   var now = new Date().getTime();
   nwDataSet0.find({ id: id, ts: { $gt: (now - 2 * 24 * 60 * 60 * 1000) } }).toArray(function (err, items) {
      if (err) {
         res.statusCode = 400;
         res.send("400");
      }
      else {
         res.statusCode = 200;
         res.type('application/json');
         res.json(items);
      }
   });
}

function appFrontGetNwDataSet1Handler(req, res) {
   var now = new Date().getTime();
   nwDataSet1.find({ ts: { $gt: (now - 2 * 24 * 60 * 60 * 1000) } }).toArray(function (err, items) {
      if (err) {
         res.statusCode = 400;
         res.send("400");
      }
      else {
         res.statusCode = 200;
         res.type('application/json');
         res.json(items);
      }
   });
}

function appFrontGetNwDataSet1IdHandler(req, res) {
   var id = +req.params.id;
   var now = new Date().getTime();
   nwDataSet1.find({ id: id, ts: { $gt: (now - 2 * 24 * 60 * 60 * 1000) } }).toArray(function (err, items) {
      if (err) {
         res.statusCode = 400;
         res.send("400");
      }
      else {
         res.statusCode = 200;
         res.type('application/json');
         res.json(items);
      }
   });
}

function appFrontGetNwDataSet2Handler(req, res) {
   // var now = new Date().getTime();
   nwDataSet2.find({}).toArray(function (err, items) {
      if (err) {
         res.statusCode = 400;
         res.send("400");
      }
      else {
         res.statusCode = 200;
         res.type('application/json');
         res.json(items);
      }
   });
}

function appFrontGetNwDataSet2IdHandler(req, res) {
   var id = +req.params.id;
   var now = new Date().getTime();
   nwDataSet2.find({ id: id, _id: { $gt: (now - 2 * 24 * 60 * 60 * 1000) } }).toArray(function (err, items) {
      if (err) {
         res.statusCode = 400;
         res.send("400");
      }
      else {
         res.statusCode = 200;
         res.type('application/json');
         res.json(items);
      }
   });
}

function appFrontGetLatencyHandler(req, res) {
   var now = new Date().getTime();
   latency.find({ ts: { $gt: (now - 2 * 24 * 60 * 60 * 1000) } }).toArray(function (err, items) {
      if (err) {
         res.statusCode = 400;
         res.send("400");
      }
      else {
         res.statusCode = 200;
         res.type('application/json');
         res.json(items);
      }
   });
}

function appFrontGetLatencyIdHandler(req, res) {
   var id = +req.params.id;
   var now = new Date().getTime();
   latency.find({ id: id, ts: { $gt: (now - 2 * 24 * 60 * 60 * 1000) } }).toArray(function (err, items) {
      if (err) {
         res.statusCode = 400;
         res.send("400");
      }
      else {
         res.statusCode = 200;
         res.type('application/json');
         res.json(items);
      }
   });
}
//================================================================================
//Event handlers ends
//================================================================================
//================================================================================
//Functions start
//================================================================================
function dbPurge(dbcol) {
   dbcol.count({}, function (error, numOfDocs) {
      //console.log("+=+=dbcount:"+numOfDocs);
      if (numOfDocs > settings.maxNumDbItems) {
         dbcol.find({}).toArray(function (err, items) {
            console.log("!=!=!=!=dbPurge:" + items.length + ":" + settings.maxNumDbItems);
            if (!err && items.length > settings.maxNumDbItems) {
               var delCount = Math.floor(items.length / 5) + 1;
               console.log("!=!=!=delete:" + delCount);
               dbcol.remove({ _id: { $lt: items[delCount]._id } });
               //db.repairDatabase();
            }
         });
      }
   });
}
//================================================================================
function webbroadcast(data) {
   for (var i in this.clients) {
      try {
         this.clients[i].send(data);
      }
      catch (err) {
         console.log("Client send fail");
      }
   }
}
//================================================================================
// EXCEPTION HANDLER AND ERROR LOGGING
//================================================================================
function logError(error) {
   if (!log.hasOwnProperty(session)) {
      log[session] = [];
   }

   log[session].push(error);

   fs.writeFile("./log.json", JSON.stringify(log, null, 4), function (err) {
      if (err) {
         console.log("couldn't write errorLog.json file");
      }
   });
   console.log("error: " + error);
   console.trace();
}

// function timeStamp(t) {
//    var now;
//    if (t === undefined)
//       now = new Date();
//    else
//       now = new Date(t);

//    var date = [now.getMonth() + 1, now.getDate(), now.getFullYear()];
//    var time = [now.getHours(), now.getMinutes(), now.getSeconds()];
//    var suffix = (time[0] < 12) ? "AM" : "PM";
//    time[0] = (time[0] < 12) ? time[0] : time[0] - 12;
//    time[0] = time[0] || 12;
//    for (var i = 1; i < 3; i++) {
//       if (time[i] < 10) {
//          time[i] = "0" + time[i];
//       }
//    }
//    return date.join("/") + " " + time.join(":") + " " + suffix;
// }

function timeStampInHMS(t) {
   var now;
   if (t === undefined)
      now = new Date();
   else
      now = new Date(t);

   var time = [now.getHours(), now.getMinutes(), now.getSeconds()];

   return time.join(":");
}


//================================================================================
// HELPER FUNCTIONS
//================================================================================


function freeCoapMemory(coap_observer) {
   console.log("freeCoapMemory() enter");
   if (coap_observer) {
      if (coap_observer.coap_client && !coap_observer.coap_client.deleted) {
         coap_observer.coap_client.freeCoapClientMemory();
         coap_observer.coap_client.deleted = true;
      }
   }

   console.log("freeCoapMemory() exit");
}

// function getRandomInt(min, max) {
//    return Math.floor(Math.random() * (max - min + 1)) + min;
// }

function getMACAddress() {
   var mac = "00:00:00:00:00:00";
   var devs = fs.readdirSync('/sys/class/net/');
   devs.forEach(function (dev) {
      var fn = path.join('/sys/class/net', dev, 'address');
      //if(dev.substr(0, 4) == 'eth0' && fs.existsSync(fn)) {
      if (dev.substr(0, 6) == 'enp0s4' && fs.existsSync(fn)) {
         mac = fs.readFileSync(fn)
            .toString()
            .trim();
      }
   });
   return mac;
}

// function getIPAddress() {
//    var ip;
//    var ifaces = os.networkInterfaces();
//    if ('lpn0' in ifaces) {
//       ifaces['lpn0'].some(function (details) {
//          if (details.family == 'IPv6') {
//             ip = details.address;
//             if (ip.match(/^[23]/i) != null) {
//                return true;   // global unicast IPv6 address
//             }
//             if (ip.match(/^fd/i) != null) {
//                return true;   // unique local IPv6 address
//             }
//          }
//          return false;
//       });
//    }
//    return ip;
// }

function setConfig(gwUpdate) {

   var proxy;
   if (gwUpdate.hasOwnProperty('proxy')) {
      proxy = gwUpdate['proxy'];
   }
   else {
      proxy = '';
   }

   if (gwUpdate.hasOwnProperty('keys')) {
      if (gwUpdate['keys'].length === 0 || !gwUpdate['keys'].trim()) {
         keys = [];
      }
      else {
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
   fs.writeFileSync("settings.json", JSON.stringify(settings, null, '\t'), "utf8");
}

//================================================================================
// BACK-END - HELPER FUNCTIONS
//================================================================================

// function getNtfy(eui, callback) {
//    db.collection('notifications', function (err, collection) {
//       if (!err) {
//          collection.find({ eui: eui }).toArray(function (err, items) {
//             if (!err) {
//                return (callback(0, items[0].ntfy));
//             }
//             else {
//                logError(err);
//                return (callback(-1));
//             }
//          });
//       }
//       else {
//          logError(err);
//          return (callback(-1));
//       }
//    });
// }

// function newNetwork(networkinfo, callback) {
//    //required fields check
//    if (!networkinfo.hasOwnProperty('netname') || !isNaN(networkinfo.netname) ||
//       !networkinfo.hasOwnProperty('type') || isNaN(networkinfo.type) ||
//       !networkinfo.hasOwnProperty('ver') || isNaN(networkinfo.ver) ||
//       !networkinfo.hasOwnProperty('mla') || isNaN(networkinfo.mla) ||
//       !networkinfo.hasOwnProperty('panid') || isNaN(networkinfo.panid) ||
//       !networkinfo.hasOwnProperty('msa') || isNaN(networkinfo.msa) ||
//       !networkinfo.hasOwnProperty('prefix') || !isNaN(networkinfo.prefix) ||
//       !networkinfo.hasOwnProperty('gua') || !isNaN(networkinfo.gua) ||
//       !networkinfo.hasOwnProperty('mac') || !isNaN(networkinfo.mac) ||
//       !networkinfo.hasOwnProperty('key') || isNaN(networkinfo.key)) {
//       return (callback(-2));
//    }

//    //setup structure for network data storage
//    var networksStruct =
//    {
//       _id: networkinfo.mac,
//       timestamp: Date.now(),
//       info: networkinfo
//    };

//    //add networks collection (if none) and insert network (will overwrite if network already exsits)
//    db.createCollection("networks", function (err, collection) {
//       if (!err) {
//          collection.update({ _id: networksStruct._id }, networksStruct, { upsert: true }, function (err, result) {
//             if (err) {
//                logError(err);
//                return (callback(-1));
//             }
//             return (callback(0));
//          });
//       }
//       else {
//          logError(err);
//          return (callback(-1));
//       }
//    });

// }

function newNode(nui, eui, nodeinfo, callback) {
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
      !nodeinfo.hasOwnProperty('rtg_up_pnt') || isNaN(nodeinfo.rtg_up_pnt[0].addr)) {
      return (callback(-2));
   }

   //setup structure for node data storage
   var nodeStruct =
   {
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
         collection.update({ _id: eui }, nodeStruct, { upsert: true }, function (err) {
            if (err) {
               logError(err);
               return (callback(-1));
            }
         });

         //broadcast data to front-end websocket clients
         multigw.vpan_adjust_params(nodeStruct);
         ws.broadcast(JSON.stringify(nodeStruct));
         multigw.broadcastFrontEndDataToServers(JSON.stringify(nodeStruct));
         multigw.vpan_restore_params(nodeStruct);
         return (callback(0));
      }
      else {
         logError(err);
         return (callback(-1));
      }
   });

}

function updateNode(nui, eui, updateType, update, callback) {
   // var nodeStruct;

   //select network collection and update <updateType>
   db.collection("nodes", function (err, collection) {
      if (!err) {
         var updateStruct = {};
         updateStruct[updateType] = update;
         updateStruct['timestamp'] = Date.now();
         collection.update({ _id: eui }, { $set: updateStruct }, { upsert: true }, function (err) {
            if (err) {
               logError(err);
               return (callback(-1));
            }
            //console.log("update: "+result);
         });

         //broadcast data to front-end websocket clients
         updateStruct['_id'] = eui;
         updateStruct['network'] = nui;
         updateStruct['auth'] = 1;
         //console.log("broadcast: "+updateStruct);

         ws.broadcast(JSON.stringify(updateStruct));
         multigw.broadcastFrontEndDataToServers(JSON.stringify(updateStruct));
         return (callback(0));
      }
      else {
         logError(err);
         return (callback(-1));
      }
   });
}

function parseCoapGetSensors(data, node) {
   try {
      var sensorData = JSON.parse(data);
   }
   catch (e) {
      logError(e);
      return;
   }
   updateNode(node.network, node._id, 'sensor', sensorData, function (error) {
      if (error == 0) {
         console.log("CoAP: Update node - " + node.info.coap.srcAddr + "/sensors");
      }
      else if (error == -1) {
         console.log("CoAP: Update node - error updating DB");
      }
      else {
         console.log("CoAP: Update node - must specify <updateType>");
      }
   });
}

function coapObserveStop(node) {
   for (var i in coapObserver) {
      if (coapObserver[i].node == node._id) {
         if (coapObserver[i].observer && !coapObserver[i].observer.deleted) {
            coapObserver[i].observer.close();
            coapObserver[i].observer.deleted = true;
         }
         coapObserver[i].node = 0;
         console.log("CoAP Observe stopped: " + node._id);
      }
   }
}

function coapObserveStart(node) {
   console.log("CoAP: GET request to: " + node._id);
   var req = coap.request(
      {
         hostname: node.info.coap.srcAddr,
         method: 'GET',
         confirmable: false,
         observe: true,
         pathname: '/sensors',
         agent: new coap.Agent({ type: 'udp6' })
      });

   req.on('response', function (res) {
      console.log("CoAP Observe Response");
      coapObserver.push({ observer: res, node: node._id });
      res.setEncoding('utf8');
      res.on('data', function (data) {
         //console.log("CoAP Observe Data: "+data);
         parseCoapGetSensors(data, node);
      });
   });
   req.on('error', function (err) {
      console.log("CoAP Observe Error: " + err);
      req.freeCoapClientMemory();
   });

   req.on('timeout', function (err) {
      console.log("CoAP Observe timeout: " + err);
      req.freeCoapClientMemory();
   });
   req.end();
}

function obs_sensor_parse_handler(aobj, callback) {
   var id = aobj.id;
   var data = aobj.data;
   //   network_manager.updateLT(id, 120)
   if (!obs_sensor_list[id] || obs_sensor_list[id].deleted) {
      //callback();
      //console.log("################################Spurious sensor data:", id);
      // return;
   }

   var obj_data = {
      temp: 0,
      rhum: 0,
      lux: 0,
      press: 0,
      accelx: 0,
      accely: 0,
      accelz: 0,
      led: 0,
      eh: 0,
      eh1: 0,
      cc2650_active: 0,
      cc2650_sleep: 0,
      rf_tx: 0,
      rf_rx: 0,
      msp432_active: 0,
      msp432_sleep: 0,
      gpsen_active: 0,
      gpsen_sleep: 0,
      others: 0,
      sequence: 0,
      asn_stamp1: 0,
   };
   var idx = 0;
   var remainLength = data.length;
   var dataType, dataLength;
   // var tmpVal;

   while (remainLength > 0) {
      dataType = data.readUInt8(idx);
      idx += 1;
      dataLength = data.readUInt8(idx);
      idx += 1;


      switch (dataType) {
         case HCT_TLV_TYPE_TEMP:
            obj_data.temp = data.readInt16LE(idx) / 10;
            idx += 2;
            break;
         case HCT_TLV_TYPE_HUM:
            obj_data.rhum = data.readUInt16LE(idx);
            idx += 2;
            break;
         case HCT_TLV_TYPE_LIGHT:
            obj_data.lux = data.readUInt32LE(idx);
            idx += 4;
            break;
         case HCT_TLV_TYPE_PRE:
            obj_data.press = data.readUInt16LE(idx);
            idx += 2;
            break;
         case HCT_TLV_TYPE_MOT:
            obj_data.accelx = data.readInt16LE(idx) / 100;
            idx += 2;
            obj_data.accely = data.readInt16LE(idx) / 100;
            idx += 2;
            obj_data.accelz = data.readInt16LE(idx) / 100;
            idx += 2;
            break;
         case HCT_TLV_TYPE_LED:
            obj_data.led = data.readUInt8(idx);
            idx += 1;
            break;
         case HCT_TLV_TYPE_CHANNEL:
            obj_data.channel = data.readUInt8(idx);
            idx += 1;
            break;
         case HCT_TLV_TYPE_BAT:
            obj_data.bat = data.readUInt16LE(idx) / 100;
            idx += 2;
            break;
         case HCT_TLV_TYPE_EH:
            obj_data.eh = data.readUInt8(idx);
            idx += 1;
            obj_data.eh1 = data.readUInt8(idx);
            idx += 1;
            break;
         case HCT_TLV_TYPE_CC2650_POWER:
            obj_data.cc2650_active = 2;
            obj_data.cc2650_sleep = 2;
            obj_data.rf_tx = data.readUInt16LE(idx) / 100;
            idx += 2;
            obj_data.rf_rx = data.readUInt16LE(idx) / 100;
            idx += 2;
            break;
         case HCT_TLV_TYPE_MSP432_POWER:
            obj_data.msp432_active = data.readUInt16LE(idx) / 100;
            idx += 2;
            obj_data.msp432_sleep = data.readUInt16LE(idx) / 100;
            idx += 2;
            break;
         case HCT_TLV_TYPE_SENSOR_POWER:
            obj_data.gpsen_active = data.readUInt16LE(idx) / 100;
            idx += 2;
            obj_data.gpsen_sleep = data.readUInt16LE(idx) / 100;
            idx += 2;
            break;
         case HCT_TLV_TYPE_OTHER_POWER:
            obj_data.others = data.readUInt16LE(idx) / 100;
            idx += 2;
            break;
         case HCT_TLV_TYPE_SEQUENCE:
            obj_data.sequence = data.readUInt8(idx);
            idx += 1;
            break;
         case HCT_TLV_TYPE_SLOT_OFFSET:
            obj_data.sent_slot_offset = data.readUInt16LE(idx);
            console.log(timeStamp() + " sensor #" + id + " sent slot_offset: " + obj_data.sent_slot_offset)
            idx += 2;
            break;
         case HCT_TLV_TYPE_ASN_STAMP:
            // console.log("+--------------------" + id + "---------------------+")
            var cur_asn = network_manager.get_ASN()

            var asn_stamp1 = data.readUInt16LE(idx);
            obj_data.a2a_latency = ((cur_asn - asn_stamp1) & 0xffff) / 100.0;
            if (obj_data.a2a_latency > 300) {
               //300 is roughly 65536/100/2.
               //in this case the get_ASN got an old ASN
               obj_data.a2a_latency = 0;
            }
            // console.log("current asn: "+cur_asn+", packet created asn: "+asn_stamp1+", a2a latency: "+obj_data.a2a_latency)
            obj_data.asn_stamp1 = asn_stamp1;
            idx += 2;
            break;
         case HCT_TLV_TYPE_LAST_SENT_ASN:
            if (last_packet_created_asn[id] == null) last_packet_created_asn[id] = 0
            if (last_packet_a2a_latency[id] == null) last_packet_a2a_latency[id] = 0

            var last_sent_asn = data.readUInt16LE(idx)
            // console.log("sensor "+id+" last packet sent asn:", last_sent_asn)
            obj_data.last_m2m_latency = last_packet_a2a_latency[id] - (last_sent_asn - last_packet_created_asn[id]) / 100.0
            // if (ideal_m2m_latency[id] == null) {
            //    ideal_m2m_latency[id] = network_manager.get_slot_offset(id) / 100.0;
            //    ideal_m2m_latency[id].toFixed(2);
            // }
            if (obj_data.last_m2m_latency < 0 || obj_data.last_m2m_latency > 10) // || // get an old ASN
            //(obj_data.last_m2m_latency>ideal_m2m_latency[id] && obj_data.last_m2m_latency<1.27) ) // M2A delay happens
            {
               // console.log("[!] sensor "+id+" last p2a latency: "+obj_data.last_m2m_latency)
               // obj_data.last_m2m_latency = ideal_m2m_latency[id]
               obj_data.last_m2m_latency = 0
            }
            obj_data.last_uplink_latency = obj_data.last_m2m_latency
            console.log(timeStamp()+" sensor " + id + " last packet's uplink m2m latency: ", obj_data.last_m2m_latency.toFixed(2))
            // console.log("+-------------------------------------------+")
            last_packet_created_asn[id] = obj_data.asn_stamp1
            last_packet_a2a_latency[id] = obj_data.a2a_latency

            idx += 2;
            break;
         case HCT_TLV_TYPE_COAP_DATA_TYPE:
            var coap_data_type = data.readUInt8(idx)
            idx += 1;
            break;
         default:
            console.log("Unknown data type. Please add it to the parser and front-end display: " + dataLength + " bytes", dataType);
            idx += dataLength;
      }
      remainLength -= (dataLength + 2);
   }

   if (obs_sensor_list[id].app_per.last_seq != -1) {
      var diff = (obj_data.sequence + 256 - obs_sensor_list[id].app_per.last_seq) % 256;
      obs_sensor_list[id].app_per.sent += diff;
      if (diff > 1) {
         obs_sensor_list[id].app_per.lost += (diff - 1);
      }
   }

   var app_per = obs_sensor_list[id].app_per.sent == 0 ? 0 : obs_sensor_list[id].app_per.lost / obs_sensor_list[id].app_per.sent;
   var msg = { _id: id, sensors: obj_data, app_per: app_per };
   // console.log("[+]"+timeStamp(), id, obj_data)
   //console.log(timeStamp()+" sensor observer data: "+id+",last_seq: "+obs_sensor_list[id].app_per.last_seq+", cur seq: "+obj_data.sequence+", app_per: "+app_per+", num lost: "+obs_sensor_list[id].app_per.lost+" num sent: "+obs_sensor_list[id].app_per.sent);
   obs_sensor_list[id].app_per.last_seq = obj_data.sequence;
   multigw.vpan_adjust_params(msg);
   ws.broadcast(JSON.stringify(msg));
   multigw.broadcastFrontEndDataToServers(JSON.stringify(msg));
   multigw.vpan_restore_params(msg);

   col_update(col, { _id: id }, { $set: msg }, { upsert: true }, true);
   var sensor_data = { type: "sensor_type_0", gateway_0: cloud.data, msg: { _id: id, data: obj_data } };
   if (multigw.vpanIsActive()) {
      sensor_data.vpan = { id: multigw.vpanId() };
      sensor_data.sensor_0._id = multigw.panAddrTovpanAddr(sensor_data.sensor_0._id);
   }
   network_manager.sendDataToCloud(1, sensor_data);

   // console.log(sensor_data)
   //record energy numbers
   if (obj_data.rf_tx != null && obj_data.rf_rx != null) {
      powerstat.add_point(id, { tx: obj_data.rf_tx, rx: obj_data.rf_rx });
   }

   callback();
}

function obs_sensor_parse(id, data) {
   if (obs_sensor_list[id]) {
      obs_sensor_list[id].freshness = Date.now();
   }
   network_manager.sequencer_add({ id: id, data: data }, obs_sensor_parse_handler);
}

function obs_nw_perf_parse(id, data) {
   var t = Date.now()
   var idx = 0;
   var nwPerfTpe = data.readUInt8(idx);

   idx += 1;
   obs_nw_perf_list[id].freshness = t;

   //console.log("[+] Network data", nwPerfTpe, "from " + id);

   if (nwPerfTpe == 0 && obs_sensor_list[id] != null && !obs_sensor_list[id].deleted) {
      var channelInfo = {};
      var totalRssi = 0;
      var totalRxRssi = 0;
      var count = 0;
      var avgRssi = 0;
      var avgRxRssi = 0;

      var rssi, rxRssi, noAck, total;
      var bitMap0_31 = 0;
      var bitMap32_63 = 0;
      var len = data.readUInt8(idx);
      idx += 1;

      // console.log("Number of channel:" + len + ", TS:" + t + ", ID:" + id);

      //if (len > 0)
      //{
      bitMap0_31 = data.readUInt32LE(idx);
      idx += 4;
      bitMap32_63 = data.readUInt32LE(idx);
      idx += 4;
      //}

      for (var i = 0; i < 64; i++) {
         var curBitMap = i < 32 ? bitMap0_31 : bitMap32_63;
         var bitMapOffset = i < 32 ? 0 : 32;
         var bit = curBitMap & (1 << (i - bitMapOffset));
         // var temp;

         if (bit > 0) {
            rssi = data.readInt8(idx);
            idx += 1;
            rxRssi = data.readInt8(idx);
            idx += 1;
            noAck = data.readUInt8(idx);
            idx += 1;
            total = data.readUInt8(idx);
            idx += 1;
            totalRssi += rssi;
            count++;
         }
         else {
            rssi = 0;
            rxRssi = 0;
            noAck = 0;
            total = 0;
         }

         channelInfo[i] = { rssi: rssi, rxRssi: rxRssi, txNoAck: noAck, txTotal: total };
      }
      var txFail = data.readUInt32LE(idx);
      idx += 4;
      var txNoAck = data.readUInt32LE(idx);
      idx += 4;
      var txTotal = data.readUInt32LE(idx);
      idx += 4;
      var rxTotal = data.readUInt32LE(idx);
      idx += 4;
      var txLengthTotal = data.readUInt32LE(idx);

      data.readUInt8(idx += 3)

      if (count > 0) {
         avgRssi = totalRssi / count;
         avgRxRssi = totalRxRssi / count;
      }

      var macTxNoAckDiff, macTxTotalDiff, macRxTotalDiff, macTxLengthTotalDiff, appLostDiff, appSentDiff;

      if (txTotal > obs_sensor_list[id].last_txInfo.macTxTotal) {
         macTxNoAckDiff = txNoAck - obs_sensor_list[id].last_txInfo.macTxNoAck;
         macTxTotalDiff = txTotal - obs_sensor_list[id].last_txInfo.macTxTotal;
         macRxTotalDiff = rxTotal - obs_sensor_list[id].last_txInfo.macRxTotal;
      }
      else {
         macTxNoAckDiff = txNoAck;
         macTxTotalDiff = txTotal;
         macRxTotalDiff = rxTotal;
      }

      if (txLengthTotal > obs_sensor_list[id].last_txInfo.macTxLengthTotal) {
         macTxLengthTotalDiff = txLengthTotal - obs_sensor_list[id].last_txInfo.macTxLengthTotal;
      }
      else {
         macTxLengthTotalDiff = txLengthTotal + 400000000 - obs_sensor_list[id].last_txInfo.macTxLengthTotal;
      }

      if (obs_sensor_list[id].app_per.sent > obs_sensor_list[id].last_txInfo.appSent) {
         appLostDiff = obs_sensor_list[id].app_per.lost - obs_sensor_list[id].last_txInfo.appLost;
         appSentDiff = obs_sensor_list[id].app_per.sent - obs_sensor_list[id].last_txInfo.appSent;
      }
      else {
         appLostDiff = obs_sensor_list[id].app_per.lost;
         appSentDiff = obs_sensor_list[id].app_per.sent;
      }

      obs_sensor_list[id].last_txInfo.macTxNoAck = txNoAck;
      obs_sensor_list[id].last_txInfo.macTxTotal = txTotal;
      obs_sensor_list[id].last_txInfo.appLost = obs_sensor_list[id].app_per.lost;
      obs_sensor_list[id].last_txInfo.appSent = obs_sensor_list[id].app_per.sent;
      obs_sensor_list[id].last_txInfo.macRxTotal = rxTotal;
      obs_sensor_list[id].last_txInfo.macTxLengthTotal = txLengthTotal;

      var network_data0 = {
         type: "network_data_0", gateway_0: cloud.data,
         msg: {
            _id: id, data: {
               ch: channelInfo, appPer: obs_sensor_list[id].app_per, txFail: txFail, txNoAck: txNoAck, txTotal: txTotal, rxTotal: rxTotal, txLengthTotal: txLengthTotal,
               macTxNoAckDiff: macTxNoAckDiff, macTxTotalDiff: macTxTotalDiff, macRxTotalDiff: macRxTotalDiff, macTxLengthTotalDiff: macTxLengthTotalDiff, appLostDiff: appLostDiff, appSentDiff: appSentDiff
            }
         }
      };

      // nwDataSet0.insert({ _id: t, id: id, ts: t, rssi: avgRssi, rxRssi: avgRxRssi, appPer: obs_sensor_list[id].app_per, txFail: txFail, txNoAck: txNoAck, txTotal: txTotal, rxTotal: rxTotal, txLengthTotal: txLengthTotal, macTxNoAckDiff: macTxNoAckDiff, macTxTotalDiff: macTxTotalDiff, macRxTotalDiff: macRxTotalDiff, macTxLengthTotalDiff: macTxLengthTotalDiff, appLostDiff: appLostDiff, appSentDiff: appSentDiff });
      //      dbPurge(nwDataSet0);

      if (multigw.vpanIsActive()) {
         network_data0.vpan = { id: multigw.vpanId() };
         network_data0.nw._id = multigw.panAddrTovpanAddr(network_data0.nw._id);
      }

      network_manager.sendDataToCloud(1, network_data0);
   }
   else if (nwPerfTpe == 1) {
      var nwInfo =
      {
         curParent: +data.readUInt16LE(idx),
         numParentChange: +data.readUInt16LE(idx += 2),
         numSyncLost: +data.readUInt16LE(idx += 2),
         avgDrift: +data.readUInt16LE(idx += 2),
         maxDrift: +data.readUInt16LE(idx += 2),
         numMacOutOfBuffer: +data.readUInt16LE(idx += 2),
         numUipRxLost: +data.readUInt16LE(idx += 2),
         numLowpanTxLost: +data.readUInt16LE(idx += 2),
         numLowpanRxLost: +data.readUInt16LE(idx += 2),
         numCoapRxLost: +data.readUInt16LE(idx += 2),
         numCoapObsDis: +data.readUInt16LE(idx += 2)
      }

      data.readUInt8(idx += 3)

      var network_data1 = { type: "network_data_1", gateway_0: cloud.data, msg: { _id: id, data: nwInfo } };
      // nwDataSet1.insert({ _id: t, id: id, msg: nwInfo, ts: t });
      //dbPurge(nwDataSet1);
      if (multigw.vpanIsActive()) {
         network_data1.vpan = { id: multigw.vpanId() };
         network_data1.nw._id = multigw.panAddrTovpanAddr(network_data1.nw._id);
      }
      network_manager.sendDataToCloud(1, network_data1);
   }
   else if (nwPerfTpe == 2) {
      var eui;
      var rssi;
      var gps = [];
      var dist = 1000;
      var i;
      //BLE beacon report
      var bleBeacons = [];
      // console.log("network performance packet 2");
      while ((data.length - idx) >= 5) {
         eui = +data.readUInt32LE(idx);
         idx += 4;
         rssi = +data.readInt8(idx);
         idx += 1;
         bleBeacons = bleBeacons.concat([{ "eui": eui.toString(16), "rssi": rssi }]);
      }

      data.readUInt8(idx += 3)

      eui = id2eui64[id];
      localizationFromRssiReport(eui, bleBeacons);

      for (i = 0; i < bleBeacons.length; i++) {
         // console.log("BLE beacon report from " + id + " [" + i + "] = { eui:" + bleBeacons[i].eui + ",rssi:" + bleBeacons[i].rssi + "}");
      }

      i = findNodeLocations(euiConvertTocompactEui(eui));
      if (i < nodeLocationArray.length) {
         gps = [nodeLocationArray[i].lat, nodeLocationArray[i].longi];
         dist = Math.sqrt(nodeLocationArray[i].x * nodeLocationArray[i].x + nodeLocationArray[i].y * nodeLocationArray[i].y);
      }

      //      nwDataSet2.insert({ _id: t, id: id, eui: eui, "bleBeacons": bleBeacons, "gps": gps, "dist": dist });
      //    dbPurge(nwDataSet2);
   }
}

function obs_start(id) {
   //   if(network_manager.get_layer(id)!=TESTING_LAYER) return
   if (multigw.isGateway(id))   //jira77gw
   {
      return;
   }
   if (!sensors_start)
      return

   col.find({ _id: id }).toArray(function (err, items) {
      if (err || items[0] == null || items[0].address == null) {
         console.log("IP address for node " + id + " found error");
         if (obs_sensor_list[id] != null) {
            freeCoapMemory(obs_sensor_list[id]);
            delete obs_sensor_list[id];
         }

         if (obs_nw_perf_list[id] != null) {
            freeCoapMemory(obs_nw_perf_list[id]);
            delete obs_nw_perf_list[id];
         }
      }
      else {
         if ((obs_sensor_list[id] == null || obs_sensor_list[id].deleted) && source_nodes.indexOf(old_id(id2eui64[id]))>-1 ) {
            console.log("sensor observer for node " + id + " starts");

            if (obs_sensor_list[id] != null) {
               freeCoapMemory(obs_sensor_list[id]);
               delete obs_sensor_list[id];
            }
            obs_sensor_list[id] = { freshness: Date.now(), app_per: { last_seq: -1, sent: 0, lost: 0 }, last_txInfo: { macTxNoAck: 0, macTxTotal: 0, macRxTotal: 0, macTxLengthTotal: 0, appLost: 0, appSent: 0 } };//lost start from -1
            var coap_client = coap.request(
               {
                  hostname: items[0].address,
                  method: 'GET',
                  confirmable: false,
                  observe: true,
                  pathname: '/sensors',
                  agent: new coap.Agent({ type: 'udp6' })
               });

            obs_sensor_list[id].coap_client = coap_client;

            coap_client.on('response', function (res) {
               obs_sensor_list[id].observer = res;
               obs_sensor_parse(id, res.payload);
               res.on('data', function (data) {
                  obs_sensor_parse(id, data);
               });
            });

            coap_client.on('error', function () {
               console.log("sensor observer for " + id + " get error");
               obs_sensor_list[id].coap_client.freeCoapClientMemory();
            });

            coap_client.on('timeout', function () {
               console.log("sensor observer for " + id + " get timeout");
               obs_sensor_list[id].coap_client.freeCoapClientMemory();
            });
            coap_client.end();
         }

         // if (obs_nw_perf_list[id] == null || obs_nw_perf_list[id].deleted) {
         //    console.log("network performance observer for node " + id + " starts");
         //    if (obs_nw_perf_list[id]) {
         //       freeCoapMemory(obs_nw_perf_list[id]);
         //       delete obs_nw_perf_list[id];
         //    }
         //    obs_nw_perf_list[id] = { freshness: Date.now() };//lost start from -1
         //    var coap_client_nw = coap.request(
         //       {
         //          hostname: items[0].address,
         //          method: 'GET',
         //          confirmable: false,
         //          observe: true,
         //          pathname: '/diagnosis',
         //          agent: new coap.Agent({ type: 'udp6' })
         //       });
         //    obs_nw_perf_list[id].coap_client = coap_client_nw;

         //    coap_client_nw.on('response', function (res) {
         //       obs_nw_perf_list[id].observer = res;
         //       obs_nw_perf_parse(id, res.payload);
         //       res.on('data', function (data) {
         //          obs_nw_perf_parse(id, data);
         //       });
         //    });

         //    coap_client_nw.on('error', function () {
         //       console.log("network performance observer for node " + id + " get error");
         //       obs_nw_perf_list[id].coap_client.freeCoapClientMemory();
         //    });

         //    coap_client_nw.on('timeout', function () {
         //       console.log("network performance observer for node " + id + " get timeout");
         //       obs_nw_perf_list[id].coap_client.freeCoapClientMemory();
         //    });
         //    coap_client_nw.end();
         // }

      }
   });
}

function obs_sensor_stop(id) {
   if (multigw.isGateway(id)) return;  //jira77gw
   if (obs_sensor_list[id] == null || obs_sensor_list[id].deleted) return;
   if (obs_sensor_list[id].observer != null) {
      obs_sensor_list[id].observer.close();
   }
   //delete obs_sensor_list[id];  //Do not actually delete the element since data can still come in even after the observer is closed.
   obs_sensor_list[id].deleted = true;  //Do not actually delete the element since data can still come in even after the observer is closed.
}

function obs_nw_perf_stop(id) {
   if (multigw.isGateway(id)) return;  //jira77gw
   if (obs_nw_perf_list[id] == null || obs_nw_perf_list[id].deleted) return;
   if (obs_nw_perf_list[id].observer != null) {
      obs_nw_perf_list[id].observer.close();
   }
   obs_nw_perf_list[id].deleted = true;
}

// function isEmpty(obj) {
//    return (Object.keys(obj)
//       .length === 0 && obj.constructor === Object);
// }

function pingNodeHandler(err, items) {
   var id = this.id;
   //   if(network_manager.get_layer(id)!=TESTING_LAYER) return
   if (err || items[0] == null) {
      console.log(" node " + id + " doesn't exist to be ping")
   }
   else {
      var req1 = coap.request({
         hostname: items[0].address,
         method: 'GET',
         confirmable: false,
         observe: false,
         pathname: '/ping',
         agent: new coap.Agent({ type: 'udp6' })
      });
      req1.on('response', () => {
         var req2 = coap.request({
            hostname: items[0].address,
            method: 'PUT',
            confirmable: false,
            observe: false,
            pathname: '/ping',
            agent: new coap.Agent({ type: 'udp6' })
         });
         var buf = Buffer.alloc(3)
         buf.writeUInt8(0x21, 0)
         buf.writeUInt16LE(1, 1)
         buf.writeUInt8(1, 2)
         req2.write(buf)

         req2.on('response', (resp2) => {
            var rtt = parseRTT(resp2.payload)
            var e2e_latency = rtt / 100
            if (e2e_latency < 0 || e2e_latency > 5)
               return
            console.log("[+] sensor " + id + " end-to-end latency (rtt): " + e2e_latency)
            var network_data2 = {
               type: "network_data_2", gateway_0: cloud.data,
               msg: { _id: id, e2e_latency: e2e_latency }
            };
            if (multigw.vpanIsActive()) {
               network_data2.vpan = { id: multigw.vpanId() };
               network_data2.nw._id = multigw.panAddrTovpanAddr(network_data2.nw._id);
            }
            network_manager.sendDataToCloud(1, network_data2);

         })
         req2.on('error', function () {
            // var t2 = new Date().getTime();
            // console.log("Ping node " + id + " timeout at " + timeStampInHMS(t2));
            req2.freeCoapClientMemory();
         });

         req2.on('timeout', function () {
            // var t2 = new Date().getTime();
            // console.log("Ping node " + id + " timeout at " + timeStampInHMS(t2));
            req2.freeCoapClientMemory();
         });
         req2.end()
      })
      req1.on('error', function () {
         // var t2 = new Date().getTime();
         // console.log("Ping node " + id + " timeout at " + timeStampInHMS(t2));
         req1.freeCoapClientMemory();
      });

      req1.on('timeout', function () {
         // var t2 = new Date().getTime();
         // console.log("Ping node " + id + " timeout at " + timeStampInHMS(t2));
         req1.freeCoapClientMemory();
      });
      req1.end()
   }
}

function parseRTT(data) {
   var idx = 0;
   var remainLength = data.length;
   var dataType, dataLength;
   while (remainLength > 0) {
      dataType = data.readUInt8(idx);
      idx += 1;
      dataLength = data.readUInt8(idx);
      idx += 1;
      switch (dataType) {
         case HCT_TLV_TYPE_RTT:
            var rtt = data.readUInt16LE(idx);
            idx += 2;
            return rtt
            break;
         case HCT_TLV_TYPE_COAP_DATA_TYPE:
            var coap_data_type = data.readUInt8(idx)
            idx += 1;
            break;
         default:
            console.log("Unknown data type. Please add it to the parser and front-end display: " + dataLength + " bytes", dataType);
            console.log(data)
            idx += dataLength;
            break;
      }
      remainLength -= (dataLength + 2);
   }
   return 0
}

function pingNode(id) {
   this.pph = pingNodeHandler;
   this.id = id;
   col.find({ _id: id }).toArray(this.pph);
}

function nvm_formatNvmArray() {
   var formatedArray = "[\n";
   for (var i = 0; i < NVM_node_array.length; i++) {
      formatedArray = formatedArray + JSON.stringify(NVM_node_array[i]) + ((i < NVM_node_array.length - 1) ? "," : "") + "\n";
   }
   formatedArray = formatedArray + "]";
   return formatedArray;
}

function nvm_writeNvmArrayToFile() {
   //console.log("Update the NVM file");
   var nvmfilename = "nvm.json";

   fs.writeFile(nvmfilename, nvm_formatNvmArray(), 'utf8', function (err) {
      if (err) {
         console.log("couldn't write " + nvmfilename + " file");
      }
   }
   );
}

function find_nvm_node_entry(euiStr) {
   var data = -1;

   if (euiStr && euiStr != undefined) {
      for (var i = 0; i < NVM_node_array.length; i++) {
         if (NVM_node_array[i].address == euiStr) {
            data = i;
            break;
         }
      }
   }

   return data;
}

function pull_nvm_params(address, euiStr, force) {
   //console.log("pull_nvm_params() enter:", euiStr);
   if (euiStr) {
      var idx = find_nvm_node_entry(euiStr);
      if (force || idx < 0 || !NVM_node_array[idx].fresh) {
         var eui = euiStr;

         if (idx < 0) {
            var nvm_entry = { address: eui, nvmparams: [], fresh: false, state: NVM_STATE_PULLING };
            NVM_node_array.push(nvm_entry);
            NVM_node_pull_index.push([0, MAX_NVMPARAM_ARRAY_SIZE]);
            idx = NVM_node_array.length - 1;
            nvm_writeNvmArrayToFile();
         }
         //console.log("pull_nvm_params(), idx:", idx);
         var coap_client = coap.request(
            {
               hostname: address,
               method: 'GET',
               confirmable: false,
               observe: false,
               pathname: '/nvmparams',
               agent: new coap.Agent({ type: 'udp6' })
            });

         coap_client.write(NVM_node_pull_index[idx][0].toString(), 'ascii');
         console.log("nvm_update(): pull:" + euiStr + "," + NVM_node_pull_index[idx][0].toString());
         coap_client.on('response', function (msg) {
            //console.log("EUI:", eui);

            if (eui) {
               var idx = find_nvm_node_entry(eui);
               //console.log("IDX:", idx);

               if (idx >= 0) {
                  var updateNvmFile = false;
                  var tlv_data = Array.from(msg.payload);
                  var nvm_entry = tlv_to_nvm_params(tlv_data);
                  var eol = ('eol' in nvm_entry);
                  //console.log(JSON.stringify(nvm_entry));
                  if (!eol) {
                     var fields = Object.keys(nvm_entry);
                     //console.log(fields);
                     var entryIdx = -1;
                     for (var i = 0; i < NVM_node_array[idx].nvmparams.length; i++) {
                        if (fields[0] in NVM_node_array[idx].nvmparams[i]) {
                           entryIdx = i;
                           break;
                        }
                     }
                     //console.log(entryIdx);

                     if (entryIdx >= 0) {
                        //console.log(entryIdx);
                        if (entryIdx == NVM_node_pull_index[idx][0]) {
                           NVM_node_array[idx].nvmparams[entryIdx] = nvm_entry;
                           (NVM_node_pull_index[idx][0])++;
                           updateNvmFile = true;
                        }
                        else {
                           console.log("Unexpected NVM entry idx:", entryIdx, ",", NVM_node_pull_index[idx][0])
                        }
                     }
                     else if (NVM_node_pull_index[idx][0] == NVM_node_array[idx].nvmparams.length) {
                        NVM_node_array[idx].nvmparams.push(nvm_entry);
                        (NVM_node_pull_index[idx][0])++;
                        updateNvmFile = true;
                     }

                     if (NVM_node_pull_index[idx][0] >= NVM_node_pull_index[idx][1]) {
                        eol = true;
                     }
                  }

                  //console.log(NVM_node_pull_index[idx][0], NVM_node_pull_index[idx][1]);

                  if (eol) {
                     if (NVM_node_array[idx].state == NVM_STATE_PULLING) {
                        NVM_node_array[idx].state = NVM_STATE_PULLING_DONE;
                     }
                     NVM_node_array[idx].fresh = true;
                     updateNvmFile = true;
                  }
                  if (updateNvmFile) {
                     nvm_writeNvmArrayToFile();
                  }
               }
            }
         });

         coap_client.on('error', function () {
            console.log("NVM params node get: error!");
            coap_client.freeCoapClientMemory();
         });

         coap_client.on('timeout', function () {
            console.log("NVM params node get: timeout!");
            coap_client.freeCoapClientMemory();
         });

         coap_client.end();
      }
   }
   //console.log("pull_nvm_params() exit:", euiStr);
}

const NVM_TLV_TYPE_MPSK = 0x01;
const NVM_TLV_TYPE_PNID = 0x02;
const NVM_TLV_TYPE_BCH0 = 0x03;
const NVM_TLV_TYPE_BCHMO = 0x04;
const NVM_TLV_TYPE_ASRQTO = 0x05;
const NVM_TLV_TYPE_SLFS = 0x06;
const NVM_TLV_TYPE_KALP = 0x07;
const NVM_TLV_TYPE_SCANI = 0x08;
const NVM_TLV_TYPE_NSSL = 0x09;
const NVM_TLV_TYPE_FXCN = 0x0A;
const NVM_TLV_TYPE_TXPW = 0x0B;
const NVM_TLV_TYPE_FXPA = 0x0C;
const NVM_TLV_TYPE_RPLDD = 0x0D;
const NVM_TLV_TYPE_COARCT = 0x0E;
const NVM_TLV_TYPE_COAPPT = 0x0F;
const NVM_TLV_TYPE_CODTPT = 0x10;
const NVM_TLV_TYPE_COAOMX = 0x11;
const NVM_TLV_TYPE_CODFTO = 0x12;
const NVM_TLV_TYPE_PHYM = 0x13;
const NVM_TLV_TYPE_DBGL = 0x14;
const NVM_TLV_TYPE_COMMIT = 0x15;
const NVM_TLV_TYPE_EOL = 0x16;

function nvm_params_to_tlv(nvmparams) {
   var data = [];

   if (nvmparams.mpsk != null) {
      data = data.concat([NVM_TLV_TYPE_MPSK, 16]);
      data = data.concat(nvmparams.mpsk);
   }

   if (nvmparams.pnid != null) {
      data = data.concat([NVM_TLV_TYPE_PNID, 2]);
      data = data.concat((nvmparams.pnid >> 8) & 0xFF);
      data = data.concat((nvmparams.pnid) & 0xFF);
   }

   if (nvmparams.bch0 != null) {
      data = data.concat([NVM_TLV_TYPE_BCH0, 1]);
      data = data.concat((nvmparams.bch0) & 0xFF);
   }

   if (nvmparams.bchmo != null) {
      data = data.concat([NVM_TLV_TYPE_BCHMO, 1]);
      data = data.concat((nvmparams.bchmo) & 0xFF);
   }

   if (nvmparams.asrqto != null) {
      data = data.concat([NVM_TLV_TYPE_ASRQTO, 2]);
      data = data.concat((nvmparams.asrqto >> 8) & 0xFF);
      data = data.concat((nvmparams.asrqto) & 0xFF);
   }

   if (nvmparams.slfs != null) {
      data = data.concat([NVM_TLV_TYPE_SLFS, 2]);
      data = data.concat((nvmparams.slfs >> 8) & 0xFF);
      data = data.concat((nvmparams.slfs) & 0xFF);
   }

   if (nvmparams.kalp != null) {
      data = data.concat([NVM_TLV_TYPE_KALP, 2]);
      data = data.concat((nvmparams.kalp >> 8) & 0xFF);
      data = data.concat((nvmparams.kalp) & 0xFF);
   }

   if (nvmparams.scani != null) {
      data = data.concat([NVM_TLV_TYPE_SCANI, 1]);
      data = data.concat((nvmparams.scani) & 0xFF);
   }

   if (nvmparams.nssl != null) {
      data = data.concat([NVM_TLV_TYPE_NSSL, 1]);
      data = data.concat((nvmparams.nssl) & 0xFF);
   }

   if (nvmparams.fxcn != null) {
      data = data.concat([NVM_TLV_TYPE_FXCN, 1]);
      data = data.concat((nvmparams.fxcn) & 0xFF);
   }

   if (nvmparams.txpw != null) {
      data = data.concat([NVM_TLV_TYPE_TXPW, 2]);
      data = data.concat((nvmparams.txpw >> 8) & 0xFF);
      data = data.concat((nvmparams.txpw) & 0xFF);
   }

   if (nvmparams.fxpa != null) {
      data = data.concat([NVM_TLV_TYPE_FXPA, 1]);
      data = data.concat((nvmparams.fxpa) & 0xFF);
   }

   if (nvmparams.rpldd != null) {
      data = data.concat([NVM_TLV_TYPE_RPLDD, 1]);
      data = data.concat((nvmparams.rpldd) & 0xFF);
   }

   if (nvmparams.coarct != null) {
      data = data.concat([NVM_TLV_TYPE_COARCT, 2]);
      data = data.concat((nvmparams.coarct >> 8) & 0xFF);
      data = data.concat((nvmparams.coarct) & 0xFF);
   }

   if (nvmparams.coappt != null) {
      data = data.concat([NVM_TLV_TYPE_COAPPT, 2]);
      data = data.concat((nvmparams.coappt >> 8) & 0xFF);
      data = data.concat((nvmparams.coappt) & 0xFF);
   }

   if (nvmparams.codtpt != null) {
      data = data.concat([NVM_TLV_TYPE_CODTPT, 2]);
      data = data.concat((nvmparams.codtpt >> 8) & 0xFF);
      data = data.concat((nvmparams.codtpt) & 0xFF);
   }

   if (nvmparams.coaomx != null) {
      data = data.concat([NVM_TLV_TYPE_COAOMX, 1]);
      data = data.concat((nvmparams.coaomx) & 0xFF);
   }

   if (nvmparams.codfto != null) {
      data = data.concat([NVM_TLV_TYPE_CODFTO, 1]);
      data = data.concat((nvmparams.codfto) & 0xFF);
   }

   if (nvmparams.phym != null) {
      data = data.concat([NVM_TLV_TYPE_PHYM, 1]);
      data = data.concat((nvmparams.phym) & 0xFF);
   }

   if (nvmparams.dbgl != null) {
      data = data.concat([NVM_TLV_TYPE_DBGL, 1]);
      data = data.concat((nvmparams.dbgl) & 0xFF);
   }

   if (nvmparams.commit != null) {
      data = data.concat([NVM_TLV_TYPE_COMMIT, 1]);
      data = data.concat((nvmparams.commit) & 0xFF);
   }

   return data;
}

function tlv_to_nvm_params(tlvdata) {
   var idx = 0;
   var nvmparams = {};
   while (idx < tlvdata.length) {
      var type = tlvdata[idx];
      idx++;
      var len = tlvdata[idx];
      idx++;
      var lenrem = tlvdata.length - idx;
      var lenact = (len <= lenrem) ? len : lenrem;
      switch (type) {
         case NVM_TLV_TYPE_MPSK:
            if (lenact == 16) {
               nvmparams.mpsk = [].concat(tlvdata.slice(idx, idx + lenact));
            }
            break;
         case NVM_TLV_TYPE_PNID:
            if (lenact == 2) {
               nvmparams.pnid = (tlvdata[idx] * 256) + tlvdata[idx + 1];
            }
            break;
         case NVM_TLV_TYPE_BCH0:
            if (lenact == 1) {
               nvmparams.bch0 = tlvdata[idx];
            }
            break;
         case NVM_TLV_TYPE_BCHMO:
            if (lenact == 1) {
               nvmparams.bchmo = tlvdata[idx];
            }
            break;
         case NVM_TLV_TYPE_ASRQTO:
            if (lenact == 2) {
               nvmparams.asrqto = (tlvdata[idx] * 256) + tlvdata[idx + 1];
            }
            break;
         case NVM_TLV_TYPE_SLFS:
            if (lenact == 2) {
               nvmparams.slfs = (tlvdata[idx] * 256) + tlvdata[idx + 1];
            }
            break;
         case NVM_TLV_TYPE_KALP:
            if (lenact == 2) {
               nvmparams.kalp = (tlvdata[idx] * 256) + tlvdata[idx + 1];
            }
            break;
         case NVM_TLV_TYPE_SCANI:
            if (lenact == 1) {
               nvmparams.scani = tlvdata[idx];
            }
            break;
         case NVM_TLV_TYPE_NSSL:
            if (lenact == 1) {
               nvmparams.nssl = tlvdata[idx];
            }
            break;
         case NVM_TLV_TYPE_FXCN:
            if (lenact == 1) {
               nvmparams.fxcn = tlvdata[idx];
            }
            break;
         case NVM_TLV_TYPE_TXPW:
            if (lenact == 2) {
               nvmparams.txpw = (tlvdata[idx] * 256) + tlvdata[idx + 1];
            }
            break;
         case NVM_TLV_TYPE_FXPA:
            if (lenact == 1) {
               nvmparams.fxpa = tlvdata[idx];
            }
            break;
         case NVM_TLV_TYPE_RPLDD:
            if (lenact == 1) {
               nvmparams.rpldd = tlvdata[idx];
            }
            break;
         case NVM_TLV_TYPE_COARCT:
            if (lenact == 2) {
               nvmparams.coarct = (tlvdata[idx] * 256) + tlvdata[idx + 1];
            }
            break;

         case NVM_TLV_TYPE_COAPPT:
            if (lenact == 2) {
               nvmparams.coappt = (tlvdata[idx] * 256) + tlvdata[idx + 1];
            }
            break;
         case NVM_TLV_TYPE_CODTPT:
            if (lenact == 2) {
               nvmparams.codtpt = (tlvdata[idx] * 256) + tlvdata[idx + 1];
            }
            break;
         case NVM_TLV_TYPE_COAOMX:
            if (lenact == 1) {
               nvmparams.coaomx = tlvdata[idx];
            }
            break;
         case NVM_TLV_TYPE_CODFTO:
            if (lenact == 1) {
               nvmparams.codfto = tlvdata[idx];
            }
            break;
         case NVM_TLV_TYPE_PHYM:
            if (lenact == 1) {
               nvmparams.phym = tlvdata[idx];
            }
            break;
         case NVM_TLV_TYPE_DBGL:
            if (lenact == 1) {
               nvmparams.dbgl = tlvdata[idx];
            }
            break;
         case NVM_TLV_TYPE_EOL:
            if (lenact == 1) {
               nvmparams.eol = tlvdata[idx];
            }
            break;
         default:
            break;
      }

      idx = idx + len;
   }
   return nvmparams;
}

function push_nvm_params(address, euiStr, nvmparams) {
   var data = null;
   var idx = find_nvm_node_entry(euiStr);
   if (idx >= 0) {
      data = Buffer.from(nvm_params_to_tlv(nvmparams));
   }

   if (data) {
      var coap_client = coap.request(
         {
            hostname: address,
            method: 'PUT',
            confirmable: false,
            observe: false,
            pathname: '/nvmparams',
            agent: new coap.Agent({ type: 'udp6' })
         });

      coap_client.on('response', function (msg) {
         //console.log("push_nvm_params response:" + msg.payload );
         var idx = find_nvm_node_entry(euiStr);
         if (idx >= 0 && NVM_node_array[idx].state == NVM_STATE_COMMITING) {
            if (msg.payload == "COMMIT_OK") {
               console.log("Commit success:", euiStr);
               NVM_node_array[idx].state = NVM_STATE_UPDATED;
               nvm_writeNvmArrayToFile();
               if (NVM_node_array[idx].address == id2eui64[multigw.gw_id])  //jira77gw
               {
                  //Root node updated, need to restart
                  setTimeout(function () {
                     console.log("Please restart the gateway after root node NVM update");
                     process.exit(0);
                  }, 1000);
               }
            }
            else {
               console.log("Commit failed: euiStr");
               NVM_node_array[idx].state = NVM_STATE_UPDATING;
               NVM_node_array[idx].fresh = false;
            }
         }
      });

      coap_client.on('error', function (msg) {
         console.log("push_nvm_params error:" + msg.payload)
         var idx = find_nvm_node_entry(euiStr);
         if (idx >= 0 && NVM_node_array[idx].state == NVM_STATE_COMMITING) {
            console.log("Commit failed:", euiStr);
            NVM_node_array[idx].state = NVM_STATE_UPDATING;
            NVM_node_array[idx].fresh = false;
         }
         coap_client.freeCoapClientMemory();
      });

      coap_client.on('timeout', function (msg) {
         console.log("push_nvm_params timeout:" + msg.payload)
         var idx = find_nvm_node_entry(euiStr);
         if (idx >= 0 && NVM_node_array[idx].state == NVM_STATE_COMMITING) {
            console.log("Commit failed:", euiStr);
            NVM_node_array[idx].state = NVM_STATE_UPDATING;
            NVM_node_array[idx].fresh = false;
         }
         coap_client.freeCoapClientMemory();
      });

      coap_client.write(data, 'binary');
      coap_client.end();
   }
}

function get_gw_address() {
   col.find({ _id: multigw.gw_id }).toArray(function (err, items)  //jira77gw
   {
      if (err || items.length <= 0 || items[0].address == null) {
         console.log("GW address not found: ");
      }
      else {
         gw_address = items[0].address;
         network_manager.new_retrieve_schedule_round();
      }
   });
}

function eui_to_id(euiStr) {
   var id = -1;
   var i;

   for (i in id2eui64) {
      if (id2eui64[i] == euiStr) {
         id = i;
         break;
      }
   }
   return id;
}

function nvm_all_child_committed(id) {
   var retVal = true;

   //console.log("###############nvm_all_child_committed(): ", id);

   if (nodeChildList[id] != undefined) {
      var i;
      for (i = 0; i < nodeChildList[id].length; i++) {
         var cid = nodeChildList[id][i];
         //console.log("#########nvm_all_child_committed check:", id, ":", cid);
         var idx = find_nvm_node_entry(id2eui64[cid]);
         if (idx >= 0 &&
            (NVM_node_array[idx].state == NVM_STATE_UPDATING ||
               NVM_node_array[idx].state == NVM_STATE_COMMITING ||
               NVM_node_array[idx].state == NVM_STATE_PULLING ||
               NVM_node_array[idx].state == NVM_STATE_PULLING_DONE
            )) {
            //console.log("id:",id," ,child:", cid, " updating");
            retVal = false;
            break;
         }
      }
   }
   //console.log("###############nvm_all_child_committed() exit");
   return retVal;
}

function nvm_update(address, euiStr, nvm, isgw) {
   //console.log('nvm_update() enter');
   var retVal = false;
   if (address && euiStr && (isgw || !restrictedEui || restrictedEui == euiStr)) {
      var idx = find_nvm_node_entry(euiStr);

      if (idx < 0 || NVM_node_array[idx].state == NVM_STATE_PULLING ||
         (!NVM_node_array[idx].fresh && nvm && nvm.str && nvm.str.length && nvm.str.length > 0)) {
         if (idx >= 0 && NVM_node_array[idx].state == NVM_STATE_INACTIVE) {
            NVM_node_array[idx].state = NVM_STATE_PULLING;
            nvm_writeNvmArrayToFile();
         }
         pull_nvm_params(address, euiStr);
         retVal = true;
      }

      if (!retVal && nvm && nvm.str && nvm.str.length && nvm.str.length > 0) {
         var i;
         for (i = 0; i < NVM_node_array[idx].nvmparams.length; i++) {
            var data = JSON.stringify(NVM_node_array[idx].nvmparams[i]);

            if (data != nvm.str[i]) {
               console.log("nvm_update(): push :" + euiStr);
               push_nvm_params(address, euiStr, nvm.nvmparams[i]);
               NVM_node_array[idx].state = NVM_STATE_UPDATING;
               NVM_node_pull_index[idx] = [i, i + 1];
               NVM_node_array[idx].fresh = false;
               nvm_writeNvmArrayToFile();
               retVal = true;
               break;
            }
         }

         if (!retVal && NVM_node_array[idx].state == NVM_STATE_UPDATING &&
            nvm_all_child_committed(eui_to_id(NVM_node_array[idx].address))) {
            console.log("nvm_update(): send commit :" + euiStr);
            push_nvm_params(address, euiStr, { commit: 1 });
            NVM_node_array[idx].state = NVM_STATE_COMMITING;
            nvm_writeNvmArrayToFile();
            retVal = true;
         }
      }

      if (!retVal && NVM_node_array[idx].state == NVM_STATE_PULLING_DONE) {
         NVM_node_array[idx].state = NVM_STATE_INACTIVE;
         nvm_writeNvmArrayToFile();
      }
   }

   //console.log('nvm_update() exit:', retVal);
   return retVal;
}

function nvm_topo_mapper() {
   var i;
   var maxid = 0;

   nodeChildList = {};

   for (i = 0; i < nvmNodeList.length; i++) {
      maxid = Math.max(maxid, nvmNodeList[i]._id);
      var parent_id = nvmNodeList[i].parent;
      //console.log('parent id:', parent_id);
      if (parent_id > 0 && !multigw.isGateway(parent_id))  //jira77gw
      {
         if (nodeChildList[parent_id] == undefined) {
            nodeChildList[parent_id] = [nvmNodeList[i]._id]
            //console.log('nodeChildList[', parent_id,'] created');
         }
         else {
            nodeChildList[parent_id].push(nvmNodeList[i]._id);
         }
      }
   }
}

function nvm_update_search_handler(err, items) {
   var nvmOpInProg = false;
   var id = this.id;

   if (err || items[0] == null) {
      if (err) {
         console.log("nvm_update_search_handler " + id + " error:" + err.toString());
      }
      else {
         console.log("nvm_update_search_handler " + id + " doesn't exist");
      }
   }
   else {
      nvmOpInProg = nvm_update(items[0].address, id2eui64[id], nvm_node);
   }

   nvm_update_current_idx++;
   nvm_search_count_down--;
   if (!nvmOpInProg && nvm_search_count_down > 0) {
      if (nvm_update_current_idx >= nvmNodeList.length) {
         nvm_update_current_idx = 0;
      }

      id = +nvmNodeList[nvm_update_current_idx]._id;
      this.id = id;
      col.find({ _id: id }).toArray(this.nvmuh);
   }
}

function nvm_read_nvm_settings(callback) {
   fs.readFile('nvm_node.json', function read(err, data) {
      nvm_node = {};
      restrictedEui = null;

      if (!err) {
         nvm_node.nvmparams = JSON.parse(data);
         nvm_node.str = [];
         if (nvm_node.nvmparams.length > 0 &&
            nvm_node.nvmparams[nvm_node.nvmparams.length - 1].eui) {
            restrictedEui = nvm_node.nvmparams[nvm_node.nvmparams.length - 1].eui;
            nvm_node.nvmparams.pop();
         }

         for (var i = 0; i < nvm_node.nvmparams.length; i++) {
            nvm_node.str.push(JSON.stringify(nvm_node.nvmparams[i]));
            //console.log("NVM_NODE:"+nvm_node.str[i]);
         }
      }
      else {
         //console.log("nvm_read_nvm_settings Err:", err);
      }

      fs.readFile('nvm_gw.json', function read(err, data) {
         nvm_gw = {};
         if (!err) {
            nvm_gw.nvmparams = JSON.parse(data);
            nvm_gw.str = [];
            if (nvm_gw.nvmparams.length > 0 &&
               nvm_gw.nvmparams[nvm_gw.nvmparams.length - 1].eui) {
               nvm_gw.nvmparams.pop();  //Ignor EUI in the gateway NVM file
            }
            for (var i = 0; i < nvm_gw.nvmparams.length; i++) {
               nvm_gw.str.push(JSON.stringify(nvm_gw.nvmparams[i]));
               //console.log("NVM_GW:"+nvm_gw.str[i]);
            }
         }
         else {
            //console.log("Err:", err);
         }

         if (callback) {
            callback();
         }
      });
   });
}

function nvm_update_timer_handler() {
   nvm_read_nvm_settings(function () {
      if (!nvm_update(gw_address, id2eui64[multigw.gw_id], nvm_gw, true))  //jira77gw
      {
         col.find({ _id: { $ne: 1 }, lifetime: { $gt: 0 } }).toArray(function (err, items) {
            if (!err && items.length > 0) {
               nvmNodeList = items;
               nvm_search_count_down = items.length;
               if (nvm_update_current_idx >= items.length) {
                  nvm_update_current_idx = 0;
               }

               var id = +items[nvm_update_current_idx]._id;
               this.nvmuh = nvm_update_search_handler;
               this.id = id;
               nvm_topo_mapper();
               col.find({ _id: id }).toArray(this.nvmuh);
            }
         });
      }
   });

}

function nvm_init() {
   fs.readFile('nvm.json', function read(err, data) {
      if (!err) {
         NVM_node_pull_index = [];
         NVM_node_array = JSON.parse(data);
         for (var i = 0; i < NVM_node_array.length; i++) {
            NVM_node_pull_index.push([0, NVM_node_array[i].nvmparams.length]);
            NVM_node_array[i].fresh = false;
            if (NVM_node_array[i].state == NVM_STATE_UPDATED ||
               NVM_node_array[i].state == NVM_STATE_PULLING ||
               NVM_node_array[i].state == NVM_STATE_PULLING_DONE) {
               NVM_node_array[i].state = NVM_STATE_INACTIVE;
            }

            if (NVM_node_array[i].state != NVM_STATE_INACTIVE) {
               NVM_node_array[i].state = NVM_STATE_UPDATING;
            }
         }

         nvm_writeNvmArrayToFile();
      }
      //console.log("NVM:"+nvm_formatNvmArray());
   });

   nvm_read_nvm_settings();

   setTimeout(get_gw_address, 10000);
   setInterval(nvm_update_timer_handler, 6000);
}

//==============================================
//Localization
//==============================================
function euiConvertTocompactEui(nodeEui) {
   var retVal = "";
   var i;
   var ceuiMsb = 0;

   for (i = 0; i < 5; i++) {
      ceuiMsb ^= parseInt(nodeEui.substring(i * 3, i * 3 + 2), 16);
   }

   retVal = ceuiMsb.toString(16);

   for (i = 5; i < 8; i++) {
      retVal += nodeEui.substring(i * 3, i * 3 + 2);
   }

   return retVal;
}

function findNodesMeta(cEuiIn) {
   var eui;
   var cEui;
   var retVal = null;

   for (eui in nodes_meta) {
      if (eui != "gateway" && nodes_meta[eui] != null) {
         cEui = euiConvertTocompactEui(eui);
         if (parseInt(cEui, 16) == parseInt(cEuiIn, 16)) {
            retVal = eui;
            break;
         }
      }
   }

   return retVal;
}

function upDateNodesMeta(nodeLocationRec) {
   var cEui;
   var eui;

   cEui = nodeLocationRec.eui;
   eui = findNodesMeta(cEui);
   if (eui) {
      nodes_meta[eui].gps[0] = nodeLocationRec.lat;
      nodes_meta[eui].gps[1] = nodeLocationRec.longi;
   }
   else {
      nodes_meta[nodeLocationRec.euiFull] =
      {
         "gps": [nodeLocationRec.lat, nodeLocationRec.longi],
         "type": "I3Mote",
         "power": "battery"
      };
   }
}

function SaveNodesMeta() {
   var eui;
   var cont;
   var firstLine;

   cont = "{\r\n";
   firstLine = true;
   for (eui in nodes_meta && nodes_meta[eui] != null) {
      if (!firstLine) {
         cont += ",\r\n";
      }
      firstLine = false;
      cont += '  "' + eui + '":' + JSON.stringify(nodes_meta[eui]);
   }
   cont += "\r\n}\r\n";

   fs.writeFileSync("./nodes_meta.json", cont, function (err) {
      if (err) {
         console.log("couldn't write nodes_meta.json file:" + err);
      }
   });
}

function findNodeLocations(eui) {
   var j;
   for (j = 0; j < nodeLocationArray.length; j++) {
      if (parseInt(nodeLocationArray[j].eui, 16) == parseInt(eui, 16)) {
         break;
      }
   }

   return (j);
}

function initLocalization() {
   var eui;
   var rec;
   var cEui;
   var j;

   nodeLocationArray = [];

   for (eui in nodes_meta) {
      if (eui != "gateway" && nodes_meta[eui] != null && nodes_meta[eui].gps != null) {
         cEui = euiConvertTocompactEui(eui);
         rec = { eui: cEui, lat: +nodes_meta[eui].gps[0], longi: +nodes_meta[eui].gps[1], euiFull: eui };
         j = findNodeLocations(cEui);
         if (j < nodeLocationArray.length) {
            nodeLocationArray[j].lat = rec.lat;
            nodeLocationArray[j].longi = rec.longi;
         }
         else {
            nodeLocationArray = nodeLocationArray.concat([rec]);
         }
      }
   }

   fillXYinLocations();
   //localizationFromRssiReport(testNodeEui, testRssiReport);
   //process.exit(0);
}

const rssiCorr = 0.0;
const rssiToDist =
   [
      { rssi: 1000, dist: 0.2 },
      { rssi: -40.0, dist: 0.5 },
      { rssi: -54.0, dist: 1.0 },
      { rssi: -58.4, dist: 2 },
      { rssi: -59.9, dist: 3 },
      { rssi: -61.3, dist: 4 },
      { rssi: -62.7, dist: 5 },
      { rssi: -64.1, dist: 6 },
      { rssi: -65.6, dist: 7 },
      { rssi: -67.0, dist: 8 },
      { rssi: -68.4, dist: 9 },
      { rssi: -69.9, dist: 10 },
      { rssi: -71.3, dist: 11 },
      { rssi: -72.7, dist: 12 },
      { rssi: -74.1, dist: 13 },
      { rssi: -75.6, dist: 14 },
      { rssi: -77.0, dist: 15 },
      { rssi: -78.4, dist: 16 },
      { rssi: -79.8, dist: 17 },
      { rssi: -81.3, dist: 18 },
      { rssi: -82.7, dist: 19 },
      { rssi: -84.1, dist: 20 },
      { rssi: -85.5, dist: 21 },
      { rssi: -87.0, dist: 22 },
      { rssi: -88.4, dist: 23 },
      { rssi: -89.8, dist: 24 },
      { rssi: -91.3, dist: 25 },
      { rssi: -98.4, dist: 30 },
      { rssi: -1000, dist: 40 }
   ];

function rssiToRadius(rssi) {

   var radius = 50; //meters
   var i, j, sl;

   for (i = 0; i < rssiToDist.length; i++) {
      if (rssi >= (rssiToDist[i].rssi - rssiCorr * rssiToDist[i].dist)) {
         j = i - 1;
         sl = (rssiToDist[i].dist - rssiToDist[j].dist) / (rssiToDist[i].rssi - rssiToDist[j].rssi + (rssiToDist[j].dist - rssiToDist[i].dist) * rssiCorr);
         radius = rssiToDist[j].dist + sl * (rssi - rssiToDist[j].rssi + rssiCorr * rssiToDist[j].dist);
         break;
      }
   }

   console.log("rssi:" + rssi + ",radius:" + radius);
   return radius;
}

function latLongiToXY(lat, longi, reflat, reflongi) {
   var earthRadius = 6371000;
   var dtor = 3.14159265 / 180;
   var tmp = earthRadius * dtor;
   var x = tmp * Math.cos(dtor * reflat) * (longi - reflongi);
   var y = tmp * (lat - reflat)
   return { x: x, y: y };
}

function xyToLatLongi(x, y, reflat, reflongi) {
   var earthRadius = 6371000;
   var dtor = 3.14159265 / 180;
   var tmp = earthRadius * dtor;
   var longi = reflongi + x / (tmp * Math.cos(dtor * reflat));
   var lat = reflat + y / tmp;
   return { lat: lat, longi: longi };
}

function fillXYinLocations() {
   var i, ifix = 0;
   var xy;
   for (i = 0; i < nodeLocationArray.length; i++) {
      if (isFixedNode(nodeLocationArray[i].eui)) {
         ifix = i;
         break;
      }
   }

   ref_lat = nodeLocationArray[ifix].lat;
   ref_longi = nodeLocationArray[ifix].longi;
   for (i = 0; i < nodeLocationArray.length; i++) {
      xy = latLongiToXY(nodeLocationArray[i].lat, nodeLocationArray[i].longi, ref_lat, ref_longi);
      nodeLocationArray[i].x = xy.x;
      nodeLocationArray[i].y = xy.y;
      //console.log("fillXYinLocations:"+JSON.stringify(nodeLocationArray[i]));
   }
}

function isFixedNode(nodeEui) {
   var i;
   for (i = 0; i < fixedNodeList.eui.length; i++) {
      if (nodeEui == fixedNodeList.eui[i]) {
         console.log("Fixed node:" + nodeEui);
         return true;
      }
   }

   return false;
}

function trilateration(nodeList) {
   var dx, dy, dist;
   var scale, r0, r1;
   var xp, dsita, sita, xp1, xp2, yp1, yp2, dist1, dist2;

   //Calculate the two intersection points of the first two node circles.
   dx = nodeList[0].x - nodeList[1].x;
   dy = nodeList[0].y - nodeList[1].y;
   dist = Math.sqrt(dx * dx + dy * dy);
   if (nodeList[0].radius + nodeList[1].radius < dist) {
      //Two cricles are too far apart to intersect.  Adjust the radius to make sure the two circles intersect
      scale = (dist - nodeList[0].radius) / nodeList[1].radius;
      nodeList[1].radius *= scale;
   }
   else if (nodeList[0].radius + dist < nodeList[1].radius) {
      //One circle is completely inside the other
      scale = (dist + nodeList[0].radius) / nodeList[1].radius;
      nodeList[1].radius *= scale;
   }

   r0 = nodeList[0].radius;
   r1 = nodeList[1].radius;
   xp = (dist * dist - r0 * r0 + r1 * r1) / (2 * dist);
   xp = Math.min(xp, r1);
   dsita = Math.acos(xp / r1);
   sita = Math.atan2(dy, dx);
   xp1 = nodeList[1].x + r1 * Math.cos(sita + dsita);
   yp1 = nodeList[1].y + r1 * Math.sin(sita + dsita);
   xp2 = nodeList[1].x + r1 * Math.cos(sita - dsita);
   yp2 = nodeList[1].y + r1 * Math.sin(sita - dsita);

   //Pick one of the two intersection points that is closest to the radius of the third node circle.
   dx = xp1 - nodeList[2].x;
   dy = yp1 - nodeList[2].y;
   dist1 = Math.abs(Math.sqrt(dx * dx + dy * dy) - nodeList[2].radius);
   dx = xp2 - nodeList[2].x;
   dy = yp2 - nodeList[2].y;
   dist2 = Math.abs(Math.sqrt(dx * dx + dy * dy) - nodeList[2].radius);
   if (dist1 > dist2) {
      xp1 = xp2;
      yp1 = yp2;
   }

   return { x: xp1, y: yp1 };
}

function localizationFromRssiReport(nodeEui, rssiReport) {
   var retVal = false;
   var nodeList = [];
   var candList = [];
   var cEui;
   var i, j;
   var nodeLoc;
   var newRefNode;
   var x, y, dx, dy, dist;
   var ll, loc;
   var nodel;

   if (rssiReport.length >= MIN_NUM_ANCHOR_NODES && nodeLocationArray.length >= MIN_NUM_ANCHOR_NODES && !isFixedNode(nodeEui)) {
      cEui = euiConvertTocompactEui(nodeEui);
      //Sort the rssiReport based on RSSI in decending order
      rssiReport.sort(function (a, b) { return (b.rssi - a.rssi); });
      for (i = 0; i < rssiReport.length; i++) {
         if (rssiReport[i].rssi < MIN_RSSI_FOR_LOCALIZATION || rssiReport[i].rssi > MAX_RSSI_FOR_LOCALIZATION) {
            continue;
         }
         //For each RSSI check if the node is in the nodeLocationArray
         j = findNodeLocations(rssiReport[i].eui);

         if (j < nodeLocationArray.length) {
            nodeLoc = nodeLocationArray[j];
            newRefNode = { eui: nodeLoc.eui, x: nodeLoc.x, y: nodeLoc.y, radius: rssiToRadius(rssiReport[i].rssi) };
            if (i == 0) {
               nodeList = [newRefNode];
            }
            else {
               //Calculate the distance to the previous nodes, if too small, put on a candidate lst, otherwise add to the node list
               x = nodeLoc.x;
               y = nodeLoc.y;
               for (j = 0; j < nodeList.length; j++) {
                  dx = x - nodeList[j].x;
                  dy = y - nodeList[j].y;
                  dist = Math.sqrt(dx * dx + dy * dy);
                  if (dist < DIST_MIN) {
                     candList = candList.concat([newRefNode]);
                     break;
                  }
               }
               if (j >= nodeList.length) {
                  nodeList = nodeList.concat([newRefNode]);
                  if (nodeList.length >= MAX_NUM_ANCHOR_NODES) {
                     break;
                  }
               }
            }
         }
      }
      //If the node list is less than MAX_NUM_ANCHOR_NODES nodes, copy from the candidate list if available
      j = 0;
      for (i = nodeList.length; i < MAX_NUM_ANCHOR_NODES; i++) {
         if (j >= candList.length) {
            break;
         }
         nodeList = nodeList.concat([candList[j]]);
         j++;
      }
      //If there are fewer then MIN_NUM_ANCHOR_NODES nodes in the node list, return with false, otherwise continue
      if (nodeList.length >= MIN_NUM_ANCHOR_NODES) {
         loc = [];
         for (i = 0; i < anchorNodes.length; i++) {
            if (nodeList.length < anchorNodes[i][2]) {
               break;
            }
            nodel = [nodeList[anchorNodes[i][0] - 1], nodeList[anchorNodes[i][1] - 1], nodeList[anchorNodes[i][2] - 1]];
            loc = loc.concat([trilateration(nodel)]);
         }

         j = loc.length / 2;
         i = Math.floor(j);
         j = (i != j) ? i : (i - 1);

         loc.sort(function (a, b) { return (a.x - b.x); });
         console.log("loc_x:" + JSON.stringify(loc));

         x = (loc[i].x + loc[j].x) / 2;
         loc.sort(function (a, b) { return (a.y - b.y); });
         console.log("loc_y:" + JSON.stringify(loc));

         y = (loc[i].y + loc[j].y) / 2;

         ll = xyToLatLongi(x, y, ref_lat, ref_longi);
         //This point is the estimated position of the new node, add/update to the nodeLocationArray array and set retVal to true
         var newNodeLoc = { eui: cEui, x: x, y: y, lat: ll.lat, longi: ll.longi, euiFull: nodeEui };
         j = findNodeLocations(cEui);
         if (j < nodeLocationArray.length) {
            nodeLocationArray[j].x = x;
            nodeLocationArray[j].y = y;
            nodeLocationArray[j].lat = ll.lat;
            nodeLocationArray[j].longi = ll.longi;
            console.log("-+-+-+ update existing location: " + JSON.stringify(nodeLocationArray[j]));
         }
         else {
            nodeLocationArray = nodeLocationArray.concat([newNodeLoc]);
            console.log("-+-+-+ new node locations: " + JSON.stringify(newNodeLoc));
         }
         upDateNodesMeta(newNodeLoc);
         SaveNodesMeta();
         retVal = true;
      }
   }
   return retVal;
}
