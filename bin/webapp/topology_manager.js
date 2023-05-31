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

const PROFILE_TRAFFIC = 0x01;
const PROFILE_ENERGY = 0x02;
const PROFILE_BATTERY_LIFE = 0x03;
const PROFILE = PROFILE_ENERGY;
function create_topology(){
  if(!(this instanceof create_topology)){
    return new create_topology();
  }
  
  var topology={};
  var traffic={};
  var energy={};
  this.topology=topology;
  this.energy=energy;

  this.load_from_db=function(db){
    var col=db.collection("nm");
    col.find({}).toArray(function(err,items){
      for (var i=0; i<items.length; ++i){
        var item=items[i];
        if(item.parent!=null){
          topology[item._id]=item.parent;
        }else if(item._id==1){
          topology[item._id]=0;
        }
      }
//      console.log(topology);
    });
  }

  this.load_from_dict=function(topo){
    for (var item in topo){
      topology[item]=topo[item];
    }
  }

  this.valid=function(){
    //check loop-free
    var rank={};
    function recursive_validate(proxy){
      if(rank[proxy]!=undefined)return false;
      rank[proxy]=true;
      for(var node in topology){
        if(topology[node]==proxy){
          if(!recursive_validate(node))return false;
        }
      }
      return true;
    }
    if(!recursive_validate(1)) {
      return false;
    }
    //check connected
    for(var node in topology){
      if(!rank[node]) {
        return false;
      }
    }
    return true;
  }

  this.equal=function(topo){
  }
  this.score=function(bat_list, dc_list){
    // TX_unicast
    const power_tx_unicast = 7.81 + 0.251 * 80 + 0.226 * 22 + 5.50 + 1.67
    // TX_broadcast
    const power_tx_broadcast = 7.81 + 0.251 * 80 + 1.67
    // RX_unicast
    const power_rx_unicast = 5.47 + 0.226 * 80 + 0.251 * 22 + 12.31 + 1.67
    // RX_broadcast
    const power_rx_broadcast = 5.47 + 0.226 * 80 + 1.67
    // RX_idle
    const power_rx_idle = 5.47 + 15.55 + 1.67
    //compute traffic

    //protocol: DIO, DAO, Beacon
    //Data: observer, ack, query
    for (var node in topology) {
      traffic[node]={
        uplink_rx:    0,
        uplink_tx:    1/30 +  1/10,
        downlink_rx:     0 +  1/10/5,
        downlink_tx:0,
        broadcast_rx: 1/1.27,
        broadcast_tx: 1/1.27/4, 
        total_rx:     2/1.27
      }
    }

    function traffic_recursive_compute(proxy){
      for(var node in topology){
        if(topology[node]==proxy){
          traffic_recursive_compute(node);
          traffic[proxy].uplink_rx+=traffic[node].uplink_tx;
          traffic[proxy].uplink_tx+=traffic[node].uplink_tx;
          traffic[proxy].downlink_tx+=traffic[node].downlink_rx;
          traffic[proxy].downlink_rx+=traffic[node].downlink_rx;
          traffic[proxy].total_rx+=1/1.27;
        }
      }
    }
    traffic_recursive_compute(1);

    for (var node in topology) {
        var power = {tx:0,rx:0,total:0};
        //TX unicast
        power.tx += power_tx_unicast * (traffic[node].uplink_tx + traffic[node].downlink_tx);
        //RX unicast
        power.rx += power_rx_unicast * (traffic[node].uplink_rx + traffic[node].downlink_rx);
        //TX broadcast
        power.tx += power_tx_broadcast * traffic[node].broadcast_tx;
        //RX broadcast
        power.rx += power_rx_broadcast * traffic[node].broadcast_rx;
        //RX idle
        power.rx += power_rx_idle * traffic[node].total_rx-(traffic[node].downlink_rx + traffic[node].uplink_rx + traffic[node].broadcast_rx);
        power.total = power.tx+power.rx;
        energy[node] = power;
    }
//    console.log(traffic);
//    console.log(energy);
    var total_energy=0;
    var max_energy =0;
    var total_traffic = 0;
    for (var node in energy){
      var p=0;
      total_traffic += (traffic[node].uplink_rx+traffic[node].uplink_tx+traffic[node].downlink_rx+traffic[node].downlink_tx);
      if(bat_list!=null){
        if(bat_list.indexOf(+node)!=-1){
          p=energy[node].total;
        }
      }else if(dc_list!=null){
        if(dc_list.indexOf(+node)==-1){
          p=energy[node].total;
        }
      }else{
        p=energy[node].total;
      }
      if(p>max_energy)max_energy=p;
      total_energy+=p;
    }
    //console.log("max:="+max);
    if(PROFILE == PROFILE_ENERGY)
      return total_energy;
    if(PROFILE == PROFILE_BATTERY_LIFE)
      return max_energy;
    if(PROFILE == PROFILE_TRAFFIC)
      return total_traffic;
  }
}
module.exports={
  create_topology:create_topology
};
