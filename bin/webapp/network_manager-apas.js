/*
 * Copyright (c) 2016, Texas Instruments Incorporated
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


var net = require('net');
var http = require('http');
var jsonSocket = require('json-socket');
var coap = require('coap');
var client = new jsonSocket(new net.Socket());
var db;
var col;
var scheduler = require('./scheduler-apas');
var sch = null;
var new_node_cb;
var nodes_meta;
var topology_manager = require('./topology_manager');
var scheduler_interface = require('./scheduler_interface');
var topology_optimize_timer = null;
var topology_interface = require('./topology_interface');
var settings = null;
var dynamic_allocation = {};

var short_topology;
try {
   short_topology = require('./mytopology.json');
   console.log("short_topology loaded");
} catch (err) {
   short_topology = null;
}

var id2eui64 = {};
var beacon_allocation_tracker = scheduler_interface.beacon_allocation_tracker;
var topo = { 1: 0 };
var subtreecount = { 1: 0 };
var sequence_list = [];

var to_cloud_sch_data = {};

const skipScheduleRetrieval = 1;
const LIFE_TIME_TICK = 1;
//const LIFE_TIME_TICK = 5;  //for test
const LIFETIME_UNIT = 5 * 1000;
// const LIFETIME      = 60;
const INFINITE_LIFETIME = 0xffff;
const MIN_LIFETIME = -450;  //Tao: increased to 30 minutes

const ASSOC_STATUS_SUCCESS = 0x00;
const ASSOC_STATUS_PAN_AT_CAPA = 0x01;
const ASSOC_STATUS_PAN_ACC_DENIED = 0x02;
const ASSOC_STATUS_PAN_ALT_PARENT = 0x03;

const LINK_OPTION_TX = 0x1
const LINK_OPTION_TX_COAP = 0x21
const LINK_OPTION_RX = 0xA
const LINK_OPTION_ADV = 0x9

const MAX_COAP_TRANSAC_COUNT = 2;     //Maximal number of concurrent CoAP operation for dynamic scheduling
const MAX_NODE_TRANSACTION_COUNT = 8;
const BEACON_CONTROL_BLANK_OUT = 60000; //Actual blank out period is between 60 - 120 seconds, average = 90 seconds
const BC_POST_DB_DELAY = 1000;  //Delay from the DB update to the beacon control to allow the DB update to complete

var UIP_CONF_DS6_NBR_NBU_ROOT = 38; //restrict 1st hop nodes
var UIP_CONF_DS6_NBR_NBU_INT = 10;
// var UIP_CONF_DS6_NBR_NBU_LEAF   = 5;
var UIP_CONF_DS6_ROUTE_NBU_ROOT = 200;
var UIP_CONF_DS6_ROUTE_NBU_INT = 10;
// var UIP_CONF_DS6_ROUTE_NBU_LEAF   = 2;
var TSCH_MAX_NUM_LINKS_ROOT = 220;
var TSCH_MAX_NUM_LINKS_INT = 30;
// var TSCH_MAX_NUM_LINKS_LEAF   = 10;
var LINK_OPTION_COAP = 33
var SelfReloadJSON = require('self-reload-json');

var topo_lock = 2;
var maxNodesPerLayer = new SelfReloadJSON('./nodes_per_layer.json');
maxNodesPerLayer.on('updated',(json)=>{console.log("update max_node_per_layer.json",json)})
var nodesPerLayer = { "-1": [1], "0": [], "1": [], "2": [], "3": [], "4": [], "5": [], "6": [], "7": [] }
var retrieve_sched_startIdx = {};
var retrieved_schedule = {};
var retrieveScheduleInProgress = false;

var beacon_control_blankout = 0;
var asn = 0;

var ghost_cell_cnt = 0;

setInterval(sendScheduleToCloud, 1000 * 60);

coap.updateTiming({
   ackTimeout: 20,
   ackRandomFactor: 5,
   maxRetransmit: 3,
   maxLatency: 30,
   piggybackReplyMs: 10
});

function addRx(sender, receiver) {
   var parent = sender
   var node = receiver
   var retRX;
   retRX=sch.find_empty_subslot([parent,node],1,{type:"downlink",layer:0});
   if(retRX!=null)
   {
      var cell = {row:retRX.row, type:"downlink",layer:0,sender:parent,receiver:node};
      sch.add_subslot(retRX.slot,retRX.subslot,cell, retRX.is_optimal);
   }
   scheduler_interface.add_link(retRX.slot.slot_offset, retRX.slot.channel_offset, parent, node, id2address(parent), id2address(node), null, true, true, normalLinkManageCallback);
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

function id2address(id) {
   return "2001:db8:1234:ffff::ff:fe00:" + id.toString(16);
}
function address2id(address) {
   return parseInt(address.substring(address.lastIndexOf(":") + 1), 16);
}

function findNodeAsync(condition, callback) {
   col.find(condition).toArray(function (err, items) {
      if (err) {
         console.log(err);
         items = [];
      }

      if (callback) {
         callback(items);
      }
   });
}

function pseudoLinkManageCallback(node, error) {
   if (error != null) {
      node_departure_process(node);
   }
}

function normalLinkManageCallback(node, error) {
   if (error != null) {
      node_departure_process(node);
   }
   else {
      if (retrieved_schedule[node]) {
         (retrieved_schedule[node].fresh)--;
      }

      new_retrieve_schedule_round();
   }
}

function removeTree(item, callback) {
   var node = item._id;
   // console.log("removeTree() enter: ", node);

   beacon_allocation_tracker[multigw.gw_id].rmRxLink = false;  //these flags are needed to deal with the race condition  the node leaves and rejoins while the messaging is going on
   beacon_allocation_tracker[multigw.gw_id].rmTxLink = true;
   scheduler_interface.rm_link(0xFFFE, 0xFFFF, multigw.gw_id, node, id2address(multigw.gw_id), id2address(node), null, false, true, normalLinkManageCallback); //Special command to the root node: delete the device
   sch.remove_node(node);
   dynamic_allocation[node] = [];
   item.parent = null;
   item.lifetime = MIN_LIFETIME;
   col_update(col, { _id: node }, { $set: item });
   if (beacon_allocation_tracker && beacon_allocation_tracker[node]) {
      delete beacon_allocation_tracker[node];
   }

   findNodeAsync({ parent: node }, function (items) {
      if (items.length > 0) {
         var i = 0;
         var rmtHandler = function () {
            i++;
            if (i < items.length) {
               removeTree(items[i], rmtHandler);
            }
            else if (callback) {
               callback();
            }
         };

         removeTree(items[i], rmtHandler);
      }
      else if (callback) {
         callback();
      }
   });

   // console.log("removeTree() exit: ", node);
}

function node_departure_handler(node, call_back_arg_obj, callback) {
   // console.log("node_departure_handler() enter 0 : ", node);
   findNodeAsync({ _id: node }, function (items) {
      //console.log("node_departure_handler() enter: ", node);
      var doCallBack = true;
      if (items.length > 0) {
         var parent = items[0].parent;
         for (var l = 0; l < 6; l++) {
            if (nodesPerLayer[l].indexOf(node) != -1) {
               nodesPerLayer[l].splice(nodesPerLayer[l].indexOf(node), 1)
            }
         }
         if (parent) {
            console.log("Node:", node, " has left, parent:", parent);

            for (var slot = 0; slot < sch.slotFrameLength; ++slot) {
               for (var c in sch.channels) {
                  var ch = sch.channels[c];
                  for (var sub = 0; sub < scheduler.SUBSLOTS; ++sub) {
                     if (sch.schedule[slot][ch][sub] != null &&
                        ((sch.schedule[slot][ch][sub].sender == node && sch.schedule[slot][ch][sub].receiver == parent) ||
                           (sch.schedule[slot][ch][sub].receiver == node && sch.schedule[slot][ch][sub].sender == parent)
                        )) {
                        (beacon_allocation_tracker[sch.schedule[slot][ch][sub].sender].linkCount)--;
                        (beacon_allocation_tracker[sch.schedule[slot][ch][sub].receiver].linkCount)--;
                        break;
                     }
                  }
               }
            }

            if (!multigw.isGateway(parent))  //clean up the parent node NHL device table, root node is cleaned in removeTree()  //jira77gw
            {
               beacon_allocation_tracker[parent].rmRxLink = false;  //these flags are needed to deal with the race condition  the node leaves and rejoins while the messaging is going on
               beacon_allocation_tracker[parent].rmTxLink = true;
               scheduler_interface.rm_link(0xFFFE, 0xFFFF, parent, node, id2address(parent), id2address(node), null, false, true, normalLinkManageCallback); //Special command to the parent node: delete the device
            }
            removeTree(items[0], function () {
               setTimeout(function () { beacon_control_all(call_back_arg_obj, callback); }, BC_POST_DB_DELAY);  //Delay to allow the database update to be completed
            });
            doCallBack = false;

            retrieve_sched_startIdx[node] = null;
            retrieved_schedule[node] = null;
         }
      }

      if (callback && doCallBack) {
         callback(call_back_arg_obj);
      }
      // console.log("node_departure_handler() exit: ", node);
   });
   // console.log("node_departure_handler() exit 0");
}

function node_departure_op(node, callback) {
   node_departure_handler(node, null, callback);
}

function node_departure_process(node) {
   sequencer_add(node, node_departure_op);
}

function lifetime_expire_handler(func_arg, callback) {
   findNodeAsync({ lifetime: { $gt: MIN_LIFETIME } }, function (items) {
      for (var i = 0; i < items.length; ++i) {
         if (items[i].lifetime < INFINITE_LIFETIME) {
            items[i].lifetime -= LIFE_TIME_TICK;
            col_update(col, { _id: items[i]._id }, { $set: items[i] });
            if (beacon_allocation_tracker[items[i]._id]) {
               beacon_allocation_tracker[items[i]._id].lt = items[i].lifetime;
            }
            // console.log("lifetime:",items[i]._id,"-", items[i].lifetime);
         }
      }

      for (i = 0; i < items.length; i++) {
         if (items[i].lifetime <= MIN_LIFETIME) {
            sequencer_add(items[i]._id, node_departure_op);
            // console.log("Node ",items[i]._id," RPL lifetime counter reach minimum");
         }
      }
      callback();
   });
}

function lifetime_counter() {
   // console.log("lifetime_counter() enter");
   sequencer_add(null, lifetime_expire_handler);
   // console.log("lifetime_counter() exit");
}

function beacon_control_blankout_timer_handler() {
   if (beacon_control_blankout > 0) {
      beacon_control_blankout--;
   }

   if (beacon_control_blankout == 0) {
      beacon_control_round();
   }
}

function setup(_host, _port, _db, _new_node_cb, _nodes_meta, _settings) {
   sequence_list = [];

   client.connect(_port, _host);
   db = _db;
   new_node_cb = _new_node_cb;
   col = db.collection('nm');
   nodes_meta = _nodes_meta;
   settings = _settings;
   //  col.remove({},{});
   id2eui64[multigw.gw_id] = "gateway"   //jira77gw
   beacon_allocation_tracker[multigw.gw_id] = { required: true, isOn: true, linkCount: 0, offStatus: "beacon on", stime: new Date().getTime(), lt: 0xffff };  //jira77gw
   col_update(col, { _id: multigw.gw_id }, {
      $set: {  //jira77gw
         _id: multigw.gw_id,
         address: id2address(multigw.gw_id),
         eui64: "gateway",
         candidate: [],
         lifetime: INFINITE_LIFETIME,
         capacity: 0xffff,
         beacon_state: beacon_allocation_tracker[multigw.gw_id].offStatus
      }
   }, { upsert: true }, true);

   var topo = {  //jira77gw
      _id: multigw.gw_id,
      address: id2address(multigw.gw_id),
      parent: null,
      eui64: null,
      gps: nodes_meta[id2eui64[multigw.gw_id]].gps,
      type: nodes_meta[id2eui64[multigw.gw_id]].type,
      power: nodes_meta[id2eui64[multigw.gw_id]].power
   }
   var topology_data = { type: "topology_data", gateway_0: cloud.data, msg: { data: topo } };
   if (multigw.vpanIsActive()) {
      topology_data.vpan = { id: multigw.vpanId() };
      topology_data.topology.data._id += multigw.vpanNodeAddressBase();
   }
   sendDataToCloud(1, topology_data);
   setInterval(lifetime_counter, LIFETIME_UNIT);
   setInterval(beacon_control_blankout_timer_handler, BEACON_CONTROL_BLANK_OUT);
}


function sequencer_callback() {
   sequence_list.splice(0, 1);
   if (sequence_list.length > 0) {
      // console.log("sequencer next:", sequence_list[0].func.name);
      sequence_list[0].func(sequence_list[0].func_arg, sequencer_callback);
   }
}

function sequencer_add(func_arg, func) {
   sequence_list.push({ func_arg: func_arg, func: func });
   if (sequence_list.length == 1) {
      // console.log("sequencer first:", sequence_list[0].func.name);ZZ
      sequence_list[0].func(sequence_list[0].func_arg, sequencer_callback);
   }
}

function beacon_off(node) {
   var transacStarted = 0;
   if (beacon_allocation_tracker && beacon_allocation_tracker[node] && beacon_allocation_tracker[node].isOn) {
      beacon_allocation_tracker[node].rmTxLink = true;
      scheduler_interface.rm_link(0xffff, 0xffff, node, 0xffff, id2address(node), id2address(0xffff), null, false, true, pseudoLinkManageCallback); //pseudo link removal, really just disable the beacon
      beacon_allocation_tracker[node].isOn = false;
      col_update(col, { _id: node }, { $set: { beacon_state: beacon_allocation_tracker[node].offStatus } }, { upsert: true }, true);
      transacStarted = 1;
   }

   return transacStarted;
}

function beacon_on(node) {
   var transacStarted = 0;
   if (beacon_allocation_tracker && beacon_allocation_tracker[node] && !(beacon_allocation_tracker[node].isOn)) {
      beacon_allocation_tracker[node].addTxLink = true;
      scheduler_interface.add_link(0xffff, 0xffff, node, 0xffff, id2address(node), id2address(0xffff), null, false, true, pseudoLinkManageCallback); //pseudo link addition, activates the beacon again
      beacon_allocation_tracker[node].isOn = true;
      col_update(col, { _id: node }, { $set: { beacon_state: beacon_allocation_tracker[node].offStatus } }, { upsert: true }, true);
      transacStarted = 1;
   }

   return transacStarted;
}

function beacon_required(node, subtreecount, topo) {
   if (beacon_allocation_tracker[node]) {
      var routMax = (node == multigw.gw_id) ? UIP_CONF_DS6_ROUTE_NBU_ROOT : UIP_CONF_DS6_ROUTE_NBU_INT;  //jira77gw
      var neighborMax = (node == multigw.gw_id) ? UIP_CONF_DS6_NBR_NBU_ROOT : UIP_CONF_DS6_NBR_NBU_INT;  //jira77gw
      var linkMax = (node == multigw.gw_id) ? TSCH_MAX_NUM_LINKS_ROOT : TSCH_MAX_NUM_LINKS_INT;  //jira77gw
      var routes = (node == multigw.gw_id) ? subtreecount[node] - 1 : beacon_allocation_tracker[node].neighborCount;  //jira77gw
      //routMax = 3; //test
      //neighborMax = (node == multigw.gw_id) ? 1 : 2; //test linear chain  //jira77gw
      //linkMax = (node == multigw.gw_id) ? 2 : TSCH_MAX_NUM_LINKS_INT;  //test  //jira77gw

      beacon_allocation_tracker[node].subtreeOff = (routes >= routMax ||
         beacon_allocation_tracker[node].linkCount >= linkMax ||
         (!multigw.isGateway(node) && beacon_allocation_tracker[topo[node]].subtreeOff));  //jira77gw

      beacon_allocation_tracker[node].required = !(beacon_allocation_tracker[node].subtreeOff ||
         beacon_allocation_tracker[node].neighborCount >= neighborMax);

      if (routes >= routMax) {
         beacon_allocation_tracker[node].offStatus = "max route";
      }
      else if (beacon_allocation_tracker[node].linkCount >= linkMax) {
         beacon_allocation_tracker[node].offStatus = "max link";
      }
      else if (beacon_allocation_tracker[node].neighborCount >= neighborMax) {
         beacon_allocation_tracker[node].offStatus = "neighbor";
      }
      else if (beacon_allocation_tracker[node].subtreeOff) {
         beacon_allocation_tracker[node].offStatus = "subtree";
      }
      else {
         beacon_allocation_tracker[node].offStatus = "beacon on";
      }

      for (var cid in topo) {
         if (topo[cid] == node) {
            beacon_required(cid, subtreecount, topo)
         }
      }
   }
}

function beacon_control(items) {
   var nodeTransacCount = 0;
   var bcDone = true;

   // console.log("beacon_control() enter");
   for (var i = 0; i < items.length; ++i) {
      // console.log("Beacon status["+items[i]._id+"]:"+items[i].beacon_state);

      //skip the new node that may not be ready to handle CoAP messages:
      if (items[i].lifetime > 0) {
         if (beacon_allocation_tracker[items[i]._id].required) {
            nodeTransacCount += beacon_on(items[i]._id);
         }
         else {
            nodeTransacCount += beacon_off(items[i]._id);
         }

         if (nodeTransacCount >= MAX_NODE_TRANSACTION_COUNT && i < (items.length - 1)) {
            bcDone = false;
            break;
         }
      }
   }

   // console.log("beacon_control() exit");
   return bcDone;
}

function beacon_control_round() {
   if (beacon_control_blankout == 0) {
      findNodeAsync({ lifetime: { $gt: MIN_LIFETIME } }, function (items) {
         if (items.length > 0) {
            beacon_control(items);
         }
      });
   }
}
function beacon_control_all(call_back_arg_obj, callback) {
   // console.log("beacon_control_all() enter 0");
   findNodeAsync({ lifetime: { $gt: MIN_LIFETIME } }, function (items) {
      // console.log("beacon_control_all() enter");
      if (items.length > 0) {
         createTopo(items);
         beacon_required(multigw.gw_id, subtreecount, topo);  //jira77gw
         if (callback) {
            callback(call_back_arg_obj);
         }
      }
      else if (callback) {
         callback(call_back_arg_obj);
      }
      // console.log("beacon_control_all() exit");
   });
   // console.log("beacon_control_all() exit 0");
}

function createTopo(items) {
   if (beacon_allocation_tracker) {
      for (var i = 0; i < items.length; ++i) {
         if (beacon_allocation_tracker[items[i]._id]) {
            beacon_allocation_tracker[items[i]._id].neighborCount = (!multigw.isGateway(items[i]._id)) ? 1 : 0;  //jira77gw
         }
      }
   }
   topo = { 1: 0 };
   subtreecount = { 1: 0 };

   for (var i = 0; i < items.length; ++i) {
      topo[items[i]._id] = items[i].parent;
      if (beacon_allocation_tracker && beacon_allocation_tracker[items[i].parent]) {
         (beacon_allocation_tracker[items[i].parent].neighborCount)++;
      }
   }
   /*
   for(var i = 0; i < items.length; ++i)
   {
      if (beacon_allocation_tracker[items[i]._id])
      {
         console.log("neighbor count["+items[i]._id+"]:"+beacon_allocation_tracker[items[i]._id].neighborCount);
      }
   }
   */
   var count_subtree = function (id) {
      var subtree = 1;
      for (var cid in topo) {
         if (topo[cid] == id) {
            subtree += count_subtree(cid);
         }
      }
      return (subtreecount[id] = subtree);
   }
   count_subtree(multigw.gw_id);  //jira77gw
}

function dynamic_schedule(callback) {
   findNodeAsync({ lifetime: { $gt: 0 } }, function (items) {
      var noCallBack = false;

      // console.log("dynamic_schedule() enter");
      if (items.length > 0) {
         var topoChanged = false;
         var coapTransacCount = 0;

         createTopo(items);

         for (var i = 0; i < items.length; ++i) {
            var node = items[i]._id;
            var parent = items[i].parent;
            var capacity = items[i].capacity;

            if (!multigw.isGateway(node) && subtreecount[node] > capacity)  //jira77gw
            {
               // console.log("Node "+node+" has "+subtreecount[node]+ " subchild nodes");
               console.log("Node " + node + " with capacity (" + capacity + ") exceeding, allocating dynamic schedule!");

               var layer = 0, thenode = parent;
               while (thenode != 1) {
                  thenode = topo[thenode]; layer++;
               }
               //Ded TX
               var retTX;
               retTX = sch.find_empty_subslot([node, parent], 1, { type: "uplink", layer: layer});
               if (retTX != null) {
                  var cell = { row: retTX.row, type: "uplink", layer: layer, sender: node, receiver: parent };
                  sch.add_subslot(retTX.slot, retTX.subslot, cell, retTX.is_optimal);
                  topoChanged = true;
               }


               //Ded RX
               var retRX;
		
               retRX=sch.find_empty_subslot([parent,node],1,{type:"downlink",layer:layer});
               if(retRX!=null)
               {
                  var cell = {row:retRX.row, type:"downlink",layer:layer,sender:parent,receiver:node};
                  sch.add_subslot(retRX.slot,retRX.subslot,cell, retRX.is_optimal);
               }
               
               if (retTX != null && retRX != null)
               {

                  (beacon_allocation_tracker[node].linkCount)++;
                  (beacon_allocation_tracker[parent].linkCount)++;
                  beacon_allocation_tracker[node].addTxLink = true;  //these flags are needed to deal with the race condition  the node leaves and rejoins while the messaging is going on
                  beacon_allocation_tracker[parent].addRxLink = true;
                  scheduler_interface.add_link(retTX.slot.slot_offset, retTX.slot.channel_offset, node, parent, id2address(node), id2address(parent), null, true, true, normalLinkManageCallback);
                  coapTransacCount++;
                  beacon_allocation_tracker[node].addRxLink = true;  //these flags are needed to deal with the race condition  the node leaves and rejoins while the messaging is going on
                  beacon_allocation_tracker[parent].addTxLink = true;
                  scheduler_interface.add_link(retRX.slot.slot_offset, retRX.slot.channel_offset, parent, node, id2address(parent), id2address(node), null, true, true, normalLinkManageCallback);

                  //update capacity
                  capacity += settings.scheduler.capacity_per_link;
                  items[i].capacity = capacity;
                  col_update(col, { _id: items[i]._id }, { $set: items[i] });
                  //store allocation
                  if (dynamic_allocation[node] == null) {
                     dynamic_allocation[node] = [];
                  }
                  dynamic_allocation[node].push([
                     { slot: retTX.slot, subslot: retTX.subslot, sender: node, receiver: parent },
                     {slot:retRX.slot, subslot:retRX.subslot, sender: parent, receiver: node}
                  ])
               }
               else {
                  console.log("Scheduler AT Capacity!");
               }

            }
            else if (!multigw.isGateway(node) && subtreecount[node] < capacity - settings.scheduler.capacity_per_link)  //jira77gw
            {
               console.log("Node " + node + " with capacity " + capacity + " wasted, deallocating dynamic schedule!");
               if (dynamic_allocation[node] != null && dynamic_allocation[node].length > 0) {
                  //update capacity
                  capacity -= settings.scheduler.capacity_per_link;
                  items[i].capacity = capacity;
                  col_update(col, { _id: items[i]._id }, { $set: items[i] });
                  var slot_list = dynamic_allocation[node][0];
                  dynamic_allocation[node].shift();
                  for (var i = 0; i < slot_list.length; ++i) {
                     var slot = slot_list[i].slot;
                     var subslot = slot_list[i].subslot;
                     var sender = slot_list[i].sender;
                     var receiver = slot_list[i].receiver;
                     sch.remove_subslot(slot, subslot);
                     topoChanged = true;
                     beacon_allocation_tracker[receiver].rmRxLink = true;  //these flags are needed to deal with the race condition  the node leaves and rejoins while the messaging is going on
                     beacon_allocation_tracker[sender].rmTxLink = true;
                     scheduler_interface.rm_link(slot.slot_offset, slot.channel_offset, sender, receiver, id2address(sender), id2address(receiver), null, true, true, normalLinkManageCallback);
                     coapTransacCount++;
                     (beacon_allocation_tracker[sender].linkCount)--;
                     (beacon_allocation_tracker[receiver].linkCount)--;
                  }
               }
               else {
                  console.log("error happens, no allocation stored");
                  if (dynamic_allocation[node] == null)
                     console.log("No dynamic allocation for this node");
                  else
                     console.log("Dynamic allocation for this node is 0");
               }
            }

            if (coapTransacCount >= MAX_COAP_TRANSAC_COUNT) {
               break;
            }
         }

         if (topoChanged) {
            setTimeout(function () { beacon_control_all(null, callback); }, BC_POST_DB_DELAY);  //Delay to allow the database update to be completed
            noCallBack = true;
         }
      }

      if (!noCallBack) {
         callback();
      }
      // console.log("dynamic_schedule() exit");
   });
}

function static_schedule(node, parent) {
   console.log("static_schedule() enter, node:", node, 'parent:', parent);
   var cell_list = [];

   var layer = 0, thenode = parent;
   while (thenode != 1) {
      thenode = topo[thenode]; layer++;
   }
   //beacon
   var period = 8

   var ret = sch.find_empty_subslot([node], period, { type: "beacon", layer: layer });

   if (ret != null) {
      var cell = { row: ret.row, type: "beacon", layer: layer, sender: node, receiver: 0xffff };
      sch.add_subslot(ret.slot, ret.subslot, cell, ret.is_optimal);
      cell_list.push({ slot: ret.slot, subslot: ret.subslot, cell });
      //  console.log("add schedule (beacon)",{slot:ret.slot,subslot:ret.subslot,cell});
      beacon_allocation_tracker[node] = { required: true, isOn: true, linkCount: 0, offStatus: "beacon on", stime: new Date().getTime(), lt: 0 };

      //ded TX
      ret = sch.find_empty_subslot([node, parent], settings.scheduler.initial_uplink_period, { type: "uplink", layer: layer });
      if (ret != null) {
         var cell = { row: ret.row, type: "uplink", layer: layer, sender: node, receiver: parent };
         sch.add_subslot(ret.slot, ret.subslot, cell, ret.is_optimal);
         cell_list.push({ slot: ret.slot, subslot: ret.subslot, cell });
         // console.log("add schedule (ded tx)",{slot:ret.slot,subslot:ret.subslot,cell});
         (beacon_allocation_tracker[node].linkCount)++;
         (beacon_allocation_tracker[parent].linkCount)++;

         //ded RX
         ret = sch.find_empty_subslot([parent, node], settings.scheduler.initial_downlink_period, { type: "downlink", layer: layer });
         if (ret != null) {
            var cell = { row: ret.row, type: "downlink", layer: layer, sender: parent, receiver: node };
            sch.add_subslot(ret.slot, ret.subslot, cell, ret.is_optimal);
            cell_list.push({ slot: ret.slot, subslot: ret.subslot, cell });
            //   console.log("add schedule (ded rx)",{slot:ret.slot,subslot:ret.subslot,cell, is_optimal:ret.is_optimal});
            (beacon_allocation_tracker[node].linkCount)++;
            (beacon_allocation_tracker[parent].linkCount)++;

            // add coap cells
            ret = sch.find_empty_subslot([node, parent], settings.scheduler.initial_downlink_period, { type: "uplink", layer: layer,option: LINK_OPTION_COAP });
            if (ret != null) {
               var cell = { row: ret.row, type: "uplink", layer: layer, sender: node, receiver: parent, option: LINK_OPTION_COAP};
               sch.add_subslot(ret.slot, ret.subslot, cell, ret.is_optimal);
               cell_list.push({ slot: ret.slot, subslot: ret.subslot, cell });
               // console.log("add schedule (ded tx)",{slot:ret.slot,subslot:ret.subslot,cell});
               (beacon_allocation_tracker[node].linkCount)++;
               (beacon_allocation_tracker[parent].linkCount)++;

               ret = sch.find_empty_subslot([parent, node], settings.scheduler.initial_downlink_period, { type: "downlink", layer: layer, option: LINK_OPTION_COAP});
               if (ret != null) {
                  var cell = { row: ret.row, type: "downlink", layer: layer, sender: parent, receiver: node,option: LINK_OPTION_COAP};
                  sch.add_subslot(ret.slot, ret.subslot, cell, ret.is_optimal);
                  cell_list.push({ slot: ret.slot, subslot: ret.subslot, cell });
                  //   console.log("add schedule (ded rx)",{slot:ret.slot,subslot:ret.subslot,cell, is_optimal:ret.is_optimal});
                  (beacon_allocation_tracker[node].linkCount)++;
                  (beacon_allocation_tracker[parent].linkCount)++;
               
            
                   // add ghost cells
                  if (ghost_cell_cnt < (sch.partition.broadcast.end - sch.partition.broadcast.start)) {
                     var remain = sch.partition.broadcast.end - ghost_cell_cnt - sch.partition.broadcast.start
                     var add = (remain > 4) ? 4 : remain
                     for (var i = sch.partition.broadcast.start + ghost_cell_cnt; i < sch.partition.broadcast.start + ghost_cell_cnt + add; i++) {
                        var slot = { slot_offset: i, channel_offset: 5 }
                        var cell = { row: 0, type: "downlink", layer: 0, sender: 1, receiver: 2, is_optimal: 1 }
                        var subslot = { period: 1, offset: 0 }
                        sch.add_subslot(slot, subslot, cell)
                        cell_list.push({ slot: slot, subslot: subslot, cell });
                        (beacon_allocation_tracker[1].linkCount)++;
                     }
                     ghost_cell_cnt += add
                  }
               }
            }
         }
      }
   }


   if (cell_list.length >= 3) {
      ret = cell_list;
      normalLinkManageCallback(node);
      normalLinkManageCallback(parent);
   }
   else {
      ret = null;
      for (var i = 0; i < cell_list.length; i++) {
         sch.remove_subslot(cell_list[i].slot, cell_list[i].subslot);
      }
      beacon_allocation_tracker[node] = null;
      cell_list = [];
   }
   console.log("static_schedule() exit");
   return ret;

}

function process_dao_handler(obj, callback) {
   var id = address2id(obj.sender);
   findNodeAsync({ _id: id }, function (items) {
      var parent = address2id(obj.parent);
      //   console.log(timeStamp()+" dao_report: "+id+" lt:"+obj.lifetime+" p:"+parent);
      if (items.length <= 0 || !items[0].parent || items[0].parent != parent) {
         console.log("spurious DAO report");
         callback();
      }
      else {
         var candidate = [];
         for (var i = 0; i < obj.candidate.length; ++i) {
            var n = address2id(obj.candidate[i]);
            if (n != 0) {
               candidate.push(n);
            }
         }
         var update = {
            _id: id,
            address: obj.sender,
            parent: parent,
            candidate: candidate,
            lifetime: obj.lifetime
         }
         col_update(col, { _id: update._id }, { $set: update }, { upsert: true });
         beacon_allocation_tracker[id].lt = obj.lifetime;
         setTimeout(function () { new_node_cb(id); }, 20000);
         dynamic_schedule(callback);

         if (settings.topology.optimizer == true) {
            //kill a previous timer
            clearTimeout(topology_optimize_timer);
            topology_optimize_timer = setTimeout(function () { topology_optimize(); }, 10000);
         }

         if (!retrieved_schedule[id] || retrieved_schedule[id].fresh < 1) {
            new_retrieve_schedule_round();
         }
      }
   });
}

function process_dao(obj) {
   sequencer_add(obj, process_dao_handler);
}

function new_node_handler(node_obj) {
   var obj = node_obj.obj;
   var client = node_obj.client;
   var node = obj.shortAddr;
   var parent = obj.coordAddr;
   var node_eui64 = obj.eui64;
   var parent_eui64 = id2eui64[parent];
   var resp;
   var layer = 0;

   // find layer
   for (var l = -1; l < 6; l++) {
      if (nodesPerLayer[l].indexOf(parent) != -1) {
         layer = l + 1
         break
      }
   }
   console.log("new_node_handler() enter: ", node, "(" + node_eui64.substr(-5) + ")", '->', parent, "(" + (parent_eui64 == "gateway" ? "gateway" : parent_eui64.substr(-5)) + ")", "at layer", layer);
   //   if(nodesPerLayer[layer].length<=maxNodesPerLayer[layer]) {
   //     console.log("layer",layer,"has space")
   //   } else {
   //     console.log("layer",layer,"has no space, try other layers")
   //   }

   //try to load specified parent from both sources
   var parent_specified_from_eui64 = nodes_meta[node_eui64] ? nodes_meta[node_eui64].parent : null;
   if (topo_lock != 1) {
      parent_specified_from_eui64 = null
   }
   var parent_specified_from_short = short_topology ? short_topology[node] : null;

   if (
      ((topo_lock == 2 && (nodesPerLayer[layer].length < maxNodesPerLayer[layer])) || topo_lock != 2)
      &&
      (
         (parent_specified_from_eui64 == null && parent_specified_from_short == null)
         ||
         (parent_specified_from_eui64 && parent_specified_from_eui64 == parent_eui64)
         ||
         (parent_specified_from_short && parent_specified_from_short == parent)
      )
   )
   //Valid topology if:
   //0. layer has space
   //1. neither specified
   //2. eui64 specified and match
   //3. short specified and match
   //eui64 has higher priority to match
   {

      if (obs_sensor_list[node] != null) {
         if (obs_sensor_list[node].observer != null)
            obs_sensor_list[node].observer.close();
         obs_sensor_list[node].deleted = true;

      }

      if (obs_nw_perf_list[node] != null) {
         if (obs_nw_perf_list[node].observer != null)
            obs_nw_perf_list[node].observer.close();
         obs_nw_perf_list[node].deleted = true;
      }

      if (!beacon_allocation_tracker[parent] || !beacon_allocation_tracker[parent].required || beacon_allocation_tracker[parent].lt <= 0) {
         //race conditions between the parent departure or turning off the beacon and the new node sneaks in, deny:1
         console.log("Parent", parent, "Capacity or LT <= 0!"); //,beacon_allocation_tracker[parent]);
         resp = [ASSOC_STATUS_PAN_ACC_DENIED, 0, 0];
      }
      else {
         console.log("beacon check pass!!!!!!!!!!!!");
         nodesPerLayer[layer].push(node)
         id2eui64[node] = obj.eui64;
         var update =
         {
            _id: node,
            address: id2address(node),
            parent: parent,
            candidate: [],
            eui64: obj.eui64,
            capacity: settings.scheduler.capacity_per_link / settings.scheduler.initial_uplink_period,
            lifetime: -1
         }
         console.log(timeStamp() + " topology accepted: " + node_eui64 + " (" + node + ") -> " + parent_eui64 + " (" + parent + "), layer " + layer);
         var cell_list = static_schedule(node, parent);

         if (cell_list == null) {
            console.log("Scheduler AT Capacity!");
            resp = [ASSOC_STATUS_PAN_AT_CAPA, 0, 0];
         }
         else {
            resp = [ASSOC_STATUS_SUCCESS, 0, cell_list.length];
            for (var i = 0; i < cell_list.length; ++i) {
               resp.push([cell_list[i].slot.slot_offset, cell_list[i].slot.channel_offset, cell_list[i].subslot.period, cell_list[i].subslot.offset])
               if (cell_list[i].cell.receiver == node) {
                  //RX link
                  resp.push(LINK_OPTION_RX)
               }
               else if (cell_list[i].cell.sender == node && cell_list[i].cell.receiver == 0xffff) {
                  //BCN link
                  resp.push(LINK_OPTION_ADV)
               }
               else {
                  //TX link
                  if(cell_list[i].cell.option == LINK_OPTION_COAP)
                     resp.push(LINK_OPTION_COAP)
                  else
                     resp.push(LINK_OPTION_TX)
               }
            }
            update.lifetime++;
            update.beacon_state = beacon_allocation_tracker[node].offStatus;
         }
         col_update(col, { _id: update._id }, { $set: update }, { upsert: true }, true);
         //  console.log("check node meta");
         if (nodes_meta[node_eui64] != null) {
            console.log("node meta check pass");
            var topo =
            {
               _id: node,
               address: id2address(node),
               parent: parent,
               eui64: obj.eui64,
               gps: nodes_meta[node_eui64].gps,
               type: nodes_meta[node_eui64].type,
               power: nodes_meta[node_eui64].power
            }

            var topology_data = { type: "topology_data", gateway_0: cloud.data, msg: { data: topo } };
            if (multigw.vpanIsActive()) {
               topology_data.vpan = { id: multigw.vpanId() };
               topology_data.topology.data._id += multigw.vpanNodeAddressBase();
            }
            console.log("send topology data to cloud");
            sendDataToCloud(1, topology_data);
         }

         beacon_control_blankout = 2;
      }
   }
   else { //Invalid topology
      var parent_id = 0;
      //check the source
      if (parent_specified_from_eui64) {
         var parent_eui64 = nodes_meta[node_eui64].parent;
         for (var id in id2eui64) {
            if (id2eui64[id] == parent_eui64) {
               parent_id = id;
               break;
            }
         }
         console.log(timeStamp() + " topology rejected with eui64 suggestion: " + node_eui64 + " (" + node + ") -> " + nodes_meta[node_eui64].parent + " (" + parent_id + ")");
         resp = [ASSOC_STATUS_PAN_ALT_PARENT, parent_id, 0];
      } else if (parent_specified_from_short) {
         parent_id = parent_specified_from_short;
         console.log(timeStamp() + " topology rejected with short suggestion: " + node_eui64 + " (" + node + ") -> " + parent_id);
         resp = [ASSOC_STATUS_PAN_ALT_PARENT, parent_id, 0];
      } else {
         for (var l = 0; l < 6; l++) {
            if (nodesPerLayer[l].length < maxNodesPerLayer[l]) {
               parent_id = nodesPerLayer[l - 1][Math.floor(Math.random() * 100 % nodesPerLayer[l - 1].length)]
               break
            }
         }
         console.log(timeStamp() + " layer " + layer + " is full, topology rejected with short suggestion: " + node_eui64 + " (" + node + ") -> " + parent_id);
         resp = [ASSOC_STATUS_PAN_ALT_PARENT, parent_id, 0];
      }
      //If parent hasn't join yet, it sends 0;
   }
   // console.log("response:",resp);
   client.sendMessage(resp);

   if (resp[0] == ASSOC_STATUS_SUCCESS) {
      setTimeout(function () { beacon_control_all(null, node_obj.callback); }, BC_POST_DB_DELAY);  //Delay to allow the database update to be completed
   }
   else {
      node_obj.callback();
   }
   console.log("new_node_handler() exit: ", node);
}

function get_layer(node) {
   var layer = 0;
   var p = topo[node]
   while (p != 1) {
      p = topo[p];
      layer++;
   }
   return layer
}

function process_new_node_handler(node_obj, callback) {
   node_obj.callback = callback;
   node_departure_handler(node_obj.obj.shortAddr, node_obj, new_node_handler);
}

function process_new_node(obj, client) {
   // console.log(timeStamp() + " new_node:" + obj.shortAddr);
   sequencer_add({ obj: obj, client: client }, process_new_node_handler);
}

function process_new_node_address_request(obj, client) {
   // console.log(timeStamp() + " new_node_addr:" + obj.shortAddr);
   var oldShortAddr = obj.shortAddr;
   var node_eui64 = obj.eui64;
   var newShortAddr = multigw.assignVpanAddress(oldShortAddr, node_eui64);
   // console.log(timeStamp() + " new_node_addr:new addr:" + newShortAddr);

   client.sendMessage(newShortAddr);
}

function process_new_gateway_address_request(obj, client) {
   client.sendMessage(multigw.gw_id);
}

function topology_optimize_handdler(func_arg, callback) {
   var topology = topology_manager.create_topology();
   findNodeAsync({ lifetime: { $gt: 0 } }, function (items) {
      if (items.length <= 0) {
         return;
      }
      var batt_list = [];
      for (var i = 0; i < items.length; ++i) {
         var item = items[i];
         if (item.parent != null) {
            topology.topology[item._id] = item.parent;
         } else if (multigw.isGateway(item._id)) {  //jira77gw
            topology.topology[item._id] = 0;
         }
         if (nodes_meta[item.eui64] == null || nodes_meta[item.eui64].power == null || nodes_meta[item.eui64].power == "Battery") {
            batt_list.push(item._id);
         }
      }
      var score = topology.score(batt_list, null);
      console.log(topology.energy);
      console.log("Topology score = " + score);

      var best_candidate = { score: score, node: null, parent: null };
      for (var i = 0; i < items.length; ++i) {
         var item = items[i];
         if (item.candidate != null) {
            for (var j = 0; j < item.candidate.length; ++j) {
               var node = item._id;
               var parent = item.candidate[j];
               var new_topology = topology_manager.create_topology();
               new_topology.load_from_dict(topology.topology);
               new_topology.topology[node] = parent;
               if (new_topology.valid()) {
                  var new_score = new_topology.score(batt_list, null);
                  //console.log(new_topology.topology)
                  //console.log("Candidate: score = "+new_score+" , change "+node+" -> "+parent);
                  if (new_score < best_candidate.score) {
                     best_candidate.score = new_score;
                     best_candidate.node = node;
                     best_candidate.parent = parent;
                  }
               }
            }
         }
      }
      if (best_candidate.node != null) {
         console.log("Best candidate found: score = " + best_candidate.score + " , change " + best_candidate.node + " -> " + best_candidate.parent);
         topology_interface.change(best_candidate.parent, id2address(best_candidate.node));
      }
      callback();
   });
}

function topology_optimize() {
   sequencer_add(null, topology_optimize_handdler);
}

client.on('message', function (obj) {
   if (obj.type == "dao_report") {
      // console.log(timeStamp()+" dao_report");
      process_dao(obj);
   } else if (obj.type == "new_node") {
      process_new_node(obj, client);
      // console.log(timeStamp() + "new node joined!!!!!!!!!!!!!!!!!!!!!");
   } else if (obj.type == "new_node_addr") {
      process_new_node_address_request(obj, client);
   } else if (obj.type == "new_gw_addr") {
      process_new_gateway_address_request(obj, client);
   } else if (obj.type == "set_minimal_config") {
      console.log("set_minimal_config:");
      sch = scheduler.create_scheduler(obj.slot_frame_size, settings.scheduler.channels);

      var p = get_partition()
      var partition_data = { type: "partition_data", gateway_0: cloud.data, msg: { data: p } };
      sendDataToCloud(1, partition_data)

      module.exports.sch = sch;
   } else if (obj.type == "dio_report") {
      //  console.log(timeStamp()+" dio_report");
   } else if (obj.type == "asn_report") {
      //console.log(timeStamp() + " asn_report:" + asn);
      asn = obj.asn;
   } else {
      console.log(timeStamp() + " unknown message " + obj.type);
   }
});

function get_schedule_old(id) {
   return sch.visualize(id);
}

function get_schedule() {
   return sch.used_subslot;
}

function get_partition() {
   var p = []
   p.push({type:"beacon", layer:0, range:[sch.partition.broadcast.start,sch.partition.broadcast.end]})
   p.push({type:"control", layer:0, range:[sch.partition.control.start,sch.partition.control.end]})
   for(var l=0;l<Object.keys(sch.partition[0].uplink).length;l++) {
      for(var r=0;r<sch.rows;r++) {
         p.push({type:'uplink',layer:l, row:r, channels:sch.partition[r].uplink[l].channels, range:[sch.partition[r].uplink[l].start,sch.partition[r].uplink[l].end]})
         p.push({type:'downlink',layer:l, row:r, channels:sch.partition[r].downlink[l].channels, range:[sch.partition[r].downlink[l].start,sch.partition[r].downlink[l].end]})
      }
   }
   // console.log(p)
   return p
 }
 

/******************************************************************************
* FUNCTION NAME:
*
* DESCRIPTION:
*
* Return Value:
* Input Parameters:
* Output Parameters:
******************************************************************************/
// function tlv_to_schedule_params(tlvdata)
// {

//    var sched_entry = {seq: 0, totalLinkCount: 0, retrievedLinkCount: 0, startLinkIdx: 0, links: []};

//    if (tlvdata.length >= 4)
//    {
//       var idx = 0;
//       var linkCount = 0;

//       sched_entry.seq = tlvdata[idx];
//       idx += 1;
//       sched_entry.totalLinkCount = tlvdata[idx];
//       idx += 1;
//       sched_entry.retrievedLinkCount = tlvdata[idx];
//       idx += 1;
//       sched_entry.startLinkIdx = tlvdata[idx];
//       idx += 1;

//       while (idx < tlvdata.length)
//       {

//          sched_entry.links[linkCount] = {};
//          var link = sched_entry.links[linkCount];

//          link.slotframe_handle = tlvdata[idx];
//          idx++;

//          if ((tlvdata.length - idx) < 1) break;
//          link.link_option = tlvdata[idx];
//          idx++;

//          if ((tlvdata.length - idx) < 1) break;
//          link.link_type = tlvdata[idx];
//          idx++;

//          if ((tlvdata.length - idx) < 1) break;
//          link.period = tlvdata[idx];
//          idx++;

//          if ((tlvdata.length - idx) < 1) break;
//          link.periodOffset = tlvdata[idx];
//          idx++;

//          if ((tlvdata.length - idx) < 2) break;
//          link.link_id = tlvdata[idx] + tlvdata[idx+1]*256;  //LE format
//          idx+=2;

//          if ((tlvdata.length - idx) < 2) break;
//          link.timeslot = tlvdata[idx] + tlvdata[idx+1]*256;  //LE format
//          idx+=2;

//          if ((tlvdata.length - idx) < 2) break;
//          link.channel_offset = tlvdata[idx] + tlvdata[idx+1]*256;  //LE format
//          idx+=2;

//          if ((tlvdata.length - idx) < 2) break;
//          link.peer_addr = tlvdata[idx] + tlvdata[idx+1]*256;  //LE format
//          idx+=2;

//          linkCount++;
//       }

//       if (linkCount != sched_entry.retrievedLinkCount || linkCount > sched_entry.totalLinkCount)
//       {
//          sched_entry.totalLinkCount = 0;
//          sched_entry.retrievedLinkCount = 0;
//       }
//    }

//    return sched_entry;
// }
/******************************************************************************
* FUNCTION NAME:
*
* DESCRIPTION:
*
* Return Value:
* Input Parameters:
* Output Parameters:
******************************************************************************/
// function retrive_schedule_node(node, callback)
// {
//    console.log("retrive_schedule_node() enter:"+node._id);
//    var startIdx;
//    var id = node._id;

//    if (!retrieve_sched_startIdx[id])
//    {
//       startIdx = retrieve_sched_startIdx[id] = 0;
//    }
//    else
//    {
//       startIdx = retrieve_sched_startIdx[id];
//    }

//    if (startIdx == 0 && retrieved_schedule[id])
//    {
//       retrieved_schedule[id].fresh = 0;  //fresh could be negative if there have been multiple requests for refresh.  Need only do once.
//    }

//    var coap_client = coap.request(
//    {
//       hostname: node.address,
//       method: 'GET',
//       confirmable: false,
//       observe: false,
//       pathname: '/schedule',
//       agent: new coap.Agent({ type: 'udp6'})
//    });

//    if (!coap_client)
//    {
//       console.log("retrieve_schedule() failed, cannot create new CoAP object");
//       if (callback != null) callback();
//    }
//    else
//    {
//       if (!retrieved_schedule[id])
//       {
//          retrieved_schedule[id] = {fresh:0, seq:0, links:[]};
//       }

//       retrieved_schedule[id].seq = (retrieved_schedule[id].seq+1) % 256;
//       var data = [];
//       data[0] = (retrieved_schedule[id].seq) & 0xFF;
//       data[1] = startIdx & 0xFF;
//       data[2] = (startIdx >> 8) & 0xFF;  //LE format
//       coap_client.write(Buffer.from(data),'binary');

//       console.log("retrieve_schedule():"+id+","+startIdx.toString());

//       var arg = {id: id, retrived_schedule: retrieved_schedule[id]};
//       sequencer_add(arg, function(arg, seq_callback)
//       {
//          var schedEntry = (arg.retrived_schedule);
//          col_update(col,{_id:arg.id},{$set:{retrived_schedule:schedEntry}},{upsert:true});
//          if (seq_callback != null) seq_callback();
//       });

//       coap_client.on('response', function(msg)
//       {
//          console.log("retrive_schedule_node() response enter");
//          if (retrieved_schedule[id])  //If the node has not departed during the wait for the response
//          {
//             retrieved_schedule[id].seq = (retrieved_schedule[id].seq + 1) % 256;

//             var tlv_data = Array.from(msg.payload);
//             var sched_entry = tlv_to_schedule_params(tlv_data);
//             if (sched_entry &&
//                 sched_entry.totalLinkCount &&
//                 sched_entry.retrievedLinkCount &&
//                 sched_entry.startLinkIdx == retrieve_sched_startIdx[id])
//             {
//                var expectedSeq = (sched_entry.seq + 1) % 256;
//                if (expectedSeq == retrieved_schedule[id].seq)
//                {
//                   retrieved_schedule[id].links = retrieved_schedule[id].links.slice(0, sched_entry.startLinkIdx);
//                   retrieved_schedule[id].links = retrieved_schedule[id].links.concat(sched_entry.links);
//                   if (sched_entry.retrievedLinkCount < sched_entry.totalLinkCount)
//                   {
//                      retrieve_sched_startIdx[id] = sched_entry.startLinkIdx + sched_entry.retrievedLinkCount;
//                      retrieve_schedule(id, retrieve_sched_startIdx[id], retrieve_schedule_round_callback);
//                   }
//                   else
//                   {
//                      (retrieved_schedule[id].fresh)++;
//                      if (retrieved_schedule[id].fresh > 0)
//                      {
//                         verify_node_slot_assignments(id);
//                      }
//                      retrieve_sched_startIdx[id] = 0;
//                      var arg = {id: id, retrived_schedule: retrieved_schedule[id]};
//                      sequencer_add(arg, function(arg, seq_callback)
//                      {
//                         var schedEntry = (arg.retrived_schedule);
//                         col_update(col,{_id:arg.id},{$set:{retrived_schedule:schedEntry}},{upsert:true});
//                         console.log('scheduled entry:',schedEntry);
//                         if (seq_callback != null) seq_callback();
//                      });
//                      console.log('retrieved schedule:',JSON.stringify(retrieved_schedule[id]));

//                      if (callback != null) callback();
//                   }
//                }
//                else
//                {
//                   console.log("Schedule retrieve from node "+id+", unexpected sequence number:"+expectedSeq+","+retrieved_schedule[id].seq);
//                   if (callback != null) callback();
//                }
//             }
//             else
//             {
//                console.log("Schedule retrieve from node "+id+", invalid retireved values:"+JSON.toString(sched_entry));
//                if (callback != null) callback();
//             }
//          }
//          else
//          {
//             console.log("Schedule retrieve from node "+id+", node departed while the retrieval was in progress");
//             if (callback != null) callback();
//          }

//          console.log("retrive_schedule_node() response exit");
//       });

//       coap_client.on('error', function()
//       {
//          console.log("Schedule retrieve from node "+id+" error!");
//          if (callback != null) callback();
//          coap_client.freeCoapClientMemory();
//       });

//       coap_client.on('timeout', function()
//       {
//          console.log("Schedule retrieve from node "+id+" timeout!");
//          if (callback != null) callback();
//          coap_client.freeCoapClientMemory();
//       });

//       coap_client.end();
//       console.log("retrive_schedule_node() exit");
//    }
// }
/******************************************************************************
* FUNCTION NAME:
*
* DESCRIPTION:
*
* Return Value:
* Input Parameters:
* Output Parameters:
******************************************************************************/
function retrieve_schedule(id, startIdx, callback) {
   console.log("retrieve_schedule() enter:" + id);
   if (startIdx != null) {
      retrieve_sched_startIdx[id] = startIdx;
   }

   findNodeAsync({ _id: id }, function (items) {
      if (items.length > 0 && items[0].address != null) {
         //retrive_schedule_node(items[0], callback)
      }
      else {
         console.log("Schedule retrieve from node " + id + " : the node does not exist:" + "," + JSON.stringify(items));
         if (callback != null) callback();
      }
   });
   console.log("retrieve_schedule() exit");
}
/******************************************************************************
* FUNCTION NAME:
*
* DESCRIPTION:
*
* Return Value:
* Input Parameters:
* Output Parameters:
******************************************************************************/
function retrieve_schedule_round_callback() {
   retrieveScheduleInProgress = false;
   new_retrieve_schedule_round();
}
/******************************************************************************
* FUNCTION NAME:
*
* DESCRIPTION:
*
* Return Value:
* Input Parameters:
* Output Parameters:
******************************************************************************/
function new_retrieve_schedule_round() {
   if (!skipScheduleRetrieval && !retrieveScheduleInProgress) {
      findNodeAsync({ lifetime: { $gt: 0 } }, function (items) {
         if (!retrieveScheduleInProgress) {
            for (var i = 0; i < items.length; i++) {
               var id = items[i]._id;
               if (!retrieved_schedule[id] || retrieved_schedule[id].fresh < 1) {
                  retrieveScheduleInProgress = true;
                  retrieve_schedule(id, 0, retrieve_schedule_round_callback);  //root node
                  break;
               }
            }
         }
      });
   }
}
/******************************************************************************
* FUNCTION NAME:
*
* DESCRIPTION:
*
* Return Value:
* Input Parameters:
* Output Parameters:
******************************************************************************/
// function retrieve_schedule_timer_handler()
// {
//    new_retrieve_schedule_round();
// }
/******************************************************************************
* FUNCTION NAME:
*
* DESCRIPTION:
*
* Return Value:
* Input Parameters:
* Output Parameters:
******************************************************************************/

// function verify_node_slot_assignments(node)
// {
//    console.log("verify_node_slot_assignments() enter: ", node);
//    var slotAssignmentOk = true;
//    var veriFailure = false;

//    for (var idx = 5; idx < retrieved_schedule[node].links.length; idx++)
//    {

//       var slot = retrieved_schedule[node].links[idx].timeslot;
//       var ch = retrieved_schedule[node].links[idx].channel_offset;
//       // var peer_addr = retrieved_schedule[node].links[idx].peer_addr;
//       var sub = sch.periodOffsetToSubslot(retrieved_schedule[node].links[idx].periodOffset, retrieved_schedule[node].links[idx].period);

//       if (sch.channels.indexOf(ch)!=-1)
//       {
//          slotAssignmentOk =  (sch.schedule[slot][ch][sub] != null &&
//                (sch.schedule[slot][ch][sub].sender == node ||
//                 sch.schedule[slot][ch][sub].receiver == node));
//       }

//       if (!slotAssignmentOk)
//       {
//          veriFailure = true;
//          if (retrieved_schedule[node].verificationFailureCount && retrieved_schedule[node].verificationFailureCount > 1)
//          {
//             console.log("########: Slot assignment error: using non-assigned slot, node: "+node+",link_id:"+retrieved_schedule[node].links[idx].link_id);

//          }
//       }
//    }

//    for(var slot = 0; slot < sch.slotFrameLength; ++slot)
//    {
//       for(var c in sch.channels)
//       {
//          var ch=sch.channels[c];
//          slotAssignmentOk = true;
//          for(var sub = 0; sub < scheduler.SUBSLOTS; ++sub)
//          {
//             if (sch.schedule[slot][ch][sub] != null &&
//                (sch.schedule[slot][ch][sub].sender == node ||
//                 sch.schedule[slot][ch][sub].receiver == node))
//             {
//                slotAssignmentOk = false;
//                for (var idx = 0; idx < retrieved_schedule[node].links.length; idx++)
//                {
//                   if (retrieved_schedule[node].links[idx].timeslot == slot &&
//                         retrieved_schedule[node].links[idx].channel_offset == ch &&
//                         sch.periodOffsetToSubslot(retrieved_schedule[node].links[idx].periodOffset, retrieved_schedule[node].links[idx].period) == sub)
//                   {
//                      slotAssignmentOk = true;
//                      break;
//                   }
//                }

//                break;
//             }
//          }

//          if (!slotAssignmentOk)
//          {
//             veriFailure = true;
//             if (retrieved_schedule[node].verificationFailureCount && retrieved_schedule[node].verificationFailureCount > 1)
//             {
//                console.log("########: Slot assignment error: Lost assigned slot, node: "+node+",slot:"+slot+",ch:"+ch+",sub:"+sub);
//             }
//          }
//       }
//    }

//    if (veriFailure)
//    {
//       if (!retrieved_schedule[node].verificationFailureCount)
//       {
//          retrieved_schedule[node].verificationFailureCount = 1;
//       }
//       else
//       {
//          retrieved_schedule[node].verificationFailureCount++;
//       }
//    }
//    else
//    {
//       retrieved_schedule[node].verificationFailureCount = 0;
//    }

//    console.log("verify_node_slot_assignments() exit");
// }

/******************************************************************************
* FUNCTION NAME:
*
* DESCRIPTION:
*
* Return Value:
* Input Parameters:
* Output Parameters:
******************************************************************************/
function sendDataToCloud(cloudOptions, cloudData) {
   cloudOptions = (typeof cloudOptions !== 'undefined') ? cloudOptions : 0;

   if (cloudOptions == 1) {
      if (cloud != null &&
         cloud.dataCenter.host != null &&
         cloud.dataCenter.path != null &&
         cloud.dataCenter.port != null &&
         cloud.data.name != "FIXME") {
         var options =
         {
            hostname: cloud.dataCenter.host,
            path: cloud.dataCenter.path,
            port: cloud.dataCenter.port,
            method: 'POST',
            headers:
            {
               'Content-Type': 'application/json',
            }
         };
         var req = http.request(options, function (res) {
            res.on('data', function () { });
         });
         req.on('timeout', function () {
            req.abort()
         })
         req.on('error', function () {
            req.abort()
         });
         req.write(JSON.stringify(cloudData));
         req.end();
      }
   }
   else if (cloudOptions == 0) {
      if (cloud != null &&
         cloud.host != null &&
         cloud.path != null &&
         cloud.port != null &&
         cloud.data.name != "FIXME") {
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

         var req = http.request(options, function (res) {
            res.on('data', function () { });
         });

         req.on('error', function () { });
         // write data to request body
         req.write(JSON.stringify(cloudData));

         //console.log("Post GW data:"+cloud.pathLocal);

         req.end();
      }
   }
}
function get_ASN() {
   return asn;
}

function sendScheduleToCloud() {
   if (sch.used_subslot.length > 0) {
      to_cloud_sch_data = { type: "schedule_data", gateway_0: cloud.data, msg: { data: sch.used_subslot } };

      sendDataToCloud(1, to_cloud_sch_data)
   }
}

// get the slot offset of a node and its 1-hop parent
function getSlotOffset(node) {
   var oneHopParent = topo[node];
   if (oneHopParent == 1) return 0

   while (topo[oneHopParent] != 1) {
      oneHopParent = topo[oneHopParent]
   }
   return findSlot(oneHopParent) - findSlot(node)
}

function findSlot(node) {
   var cell = sch.find_cell(node, "uplink");
   return cell.slot[0]
}

function set_topo_lock(flag) {
   maxNodesPerLayer.forceUpdate()
   topo_lock = flag;
}


/******************************************************************************
******************************************************************************/
module.exports = {
   setup: setup,
   get_schedule_old: get_schedule_old,
   get_schedule: get_schedule,
   get_layer: get_layer,
   get_partition: get_partition,
   sendDataToCloud: sendDataToCloud,
   sequencer_add: sequencer_add,
   new_retrieve_schedule_round: new_retrieve_schedule_round,
   node_departure_process: node_departure_process,
   get_ASN: get_ASN,
   get_slot_offset: getSlotOffset,
   set_topo_lock: set_topo_lock,
   add_rx: addRx,
};

module.exports.id2eui64 = id2eui64;
