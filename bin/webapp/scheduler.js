/*eslint-disable*/
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

/* APIs:
  this.find_empty_subslot=function(nodes_list, period, info)
  this.add_subslot=function(slot,subslot,cell)
  this.remove_node=function(node)
  this.dynamic_partition_adjustment()
*/


const RESERVED=0; //Number of reserved
const SUBSLOTS=16;
const partition_config = require("./partition.json");
const RAND = 0;
const PART = 1;
const PARTPLUS = 2;
var algorithm = PART;

var partition=null;
function partition_init(sf){
  function partition_scale(list, size){
  //scale the list to the size (sum(list)==size)
    var total=0;
    for(var i=0; i<list.length; ++i){
      total+=list[i];
    }
    for(var i=0; i<list.length; ++i){
      list[i]=list[i]*size/total;
    }
    //Now we need to arrange them to integers. We cannot directly floor or round or ceil
    //These will cause the sum!=size
    //we do the following 3 for's to round the boundary to closest integers
    for(var i=1; i<list.length; ++i){
      list[i]+=list[i-1];
    }
    for(var i=0; i<list.length; ++i){
      list[i]=Math.floor(list[i]+0.5);
    }
    for(var i=list.length-1; i>0; --i){
      list[i]-=list[i-1];
    }
    return list;
  }
  partition_config.uplink_total=0;
  partition_config.downlink_total=0;
  for(var i in partition_config.uplink){
    partition_config.uplink_total+=partition_config.uplink[i];
  }
  for(var i in partition_config.downlink){
    partition_config.downlink_total+=partition_config.downlink[i];
  }
/*  var b_u_d = [partition_config.beacon, partition_config.uplink_total, partition_config.downlink_total];
  
  partition_scale(b_u_d, sf-RESERVED-1);
  var uplink = partition_config.uplink.slice();
  partition_scale(uplink, b_u_d[1]);
  var downlink = partition_config.downlink.slice();
  partition_scale(downlink, b_u_d[2]);*/
  //Beacon reserved version
  var u_d = [partition_config.uplink_total, partition_config.downlink_total];
  partition_scale(u_d, sf-RESERVED-partition_config.beacon);
  var uplink = partition_config.uplink.slice();
  partition_scale(uplink, u_d[0]);
  var downlink = partition_config.downlink.slice();
  partition_scale(downlink, u_d[1]);

  //now we have everything scaled by slotframe length
  //and start do the partition
  var cur = RESERVED;
  var partition={};
/*  partition.broadcast={start:cur, end:cur+b_u_d[0]};
  cur+=b_u_d[0];*/

  //Beacon reserved version
  partition.broadcast={start:cur, end:cur+partition_config.beacon};
  cur+=partition_config.beacon;

  partition.uplink={};
  partition.downlink={};
  for(var i=uplink.length-1; i>0; --i){
    partition.uplink[i.toString()]={start:cur, end:cur+uplink[i]};
    cur+=uplink[i];
  }
  //FIXME: swap the placement of uplink[0] and downlink[0]
  if(algorithm == PARTPLUS){
    partition.downlink["0"]={start:cur, end:cur+downlink[0]};
    cur+=downlink[0];
    partition.uplink["0"]={start:cur, end:cur+uplink[0]};
    cur+=uplink[0];
  }else{
    partition.uplink["0"]={start:cur, end:cur+uplink[0]};
    cur+=uplink[0];
    partition.downlink["0"]={start:cur, end:cur+downlink[0]};
    cur+=downlink[0];
  }
  for(var i=1; i<downlink.length; ++i){
    partition.downlink[i.toString()]={start:cur, end:cur+downlink[i]};
    cur+=downlink[i];
  }

  console.log("patition:", partition);
  return partition;
}
function create_scheduler(sf,ch){
/*
Slot = {slot_offset, channel}
Subslot = {period, offset}
Cell = {type, sender, receiver}
*/
  // sf = 127
  if(!(this instanceof create_scheduler)){
    return new create_scheduler(sf,ch);
  }
  
  console.log("create_scheduler("+sf+","+ch+")");
  this.slotFrameLength=sf;
  this.channels=ch;
  this.schedule = new Array(sf);
  // mainly for send cells to cloud and show schedule in frontend
  this.used_subslot = []
  this.isFull = false;
  this.nonOptimalCount = 0;
  for(var i=0;i<sf;++i){
    this.schedule[i]=new Array(16);
  }

  for(var c in this.channels){
    var ch=this.channels[c];
    for(var slot=0;slot<this.slotFrameLength;++slot){
      this.schedule[slot][ch]=new Array(SUBSLOTS)
    }
  }

  //initialize partition
  this.partition = partition_init(sf);
  this.add_slot=function(slot,cell){
    this.add_subslot(slot, {offset:0, period:1}, cell);
  }

  this.add_subslot=function(slot,subslot,cell, is_optimal){
    var sub_start = subslot.offset * SUBSLOTS/subslot.period;
    var sub_end = (subslot.offset+1) * SUBSLOTS/subslot.period;
  
    this.used_subslot.push({slot:[slot.slot_offset, slot.channel_offset],subslot:[subslot.offset,subslot.period],cell:cell,is_optimal:  is_optimal})

    for(var sub = sub_start; sub < sub_end; ++sub){
      this.schedule[slot.slot_offset][slot.channel_offset][sub]={
        type:cell.type,
        layer: cell.layer,
        sender:cell.sender,
        receiver:cell.receiver,
        is_optimal: is_optimal,
      }
    }
  }

  this.remove_slot=function(slot){
    this.remove_subslot(slot, {offset:0, period:1});
  }

  this.remove_subslot=function(slot,subslot){
    var sub_start = subslot.offset * SUBSLOTS/subslot.period;
    var sub_end = (subslot.offset+1) * SUBSLOTS/subslot.period;
    for(var sub = sub_start; sub < sub_end; ++sub){
      this.schedule[slot.slot_offset][slot.channel_offset][sub] = null;
    }
    for(var i=0;i<this.used_subslot.length;i++) {
      if(this.used_subslot[i].slot[0]==slot.slot_offset &&
          this.used_subslot[i].slot[1]==slot.channel_offset &&
          this.used_subslot[i].subslot[0]==subslot.offset && 
          this.used_subslot[i].subslot[1]==subslot.period) {
        this.used_subslot.splice(i,1)
        i--
      }
    }
  }

  //3-d filter
  // flag==1, used by calc_needed_slots()
  this.available_subslot=function(nodes_list,slot,subslot,info){
    if(slot.slot_offset<RESERVED)return false;

    //if is beacon, we want it always the first channel in the list;
    //it actually doesn't matter since hardware-wise it's hardcoded to beacon channel
    //but to make it consistant in scheduler table...
    if(info.type=="beacon" && slot.channel_offset!=this.channels[0])return false;

    //Beacon reserved version: broadcast partition can only be utilized by beacon
    if(this.partition.broadcast!=null){
      var start=this.partition.broadcast.start;
      var end=this.partition.broadcast.end;
      if(info.type=="beacon"){
        if(slot.slot_offset<start||slot.slot_offset>=end)return false;
      }else{
        if(slot.slot_offset>=start&&slot.slot_offset<end)return false;
      }
    }
    
    //check if this slot-channel is assigned
    var sub_start = subslot.offset * SUBSLOTS/subslot.period;
    var sub_end = (subslot.offset+1) * SUBSLOTS/subslot.period;
    for(var sub = sub_start; sub < sub_end; ++sub){
      if(this.schedule[slot.slot_offset][slot.channel_offset][sub]!=null) {
        return false;
      }
    }
  

    //check if this slot (any channel) is assigned to beacon
    //or is assigned to the members
    for(var c in this.channels){
      var ch=this.channels[c];
      for(var sub = sub_start; sub < sub_end; ++sub){
        if(this.schedule[slot.slot_offset][ch][sub]==null)
          continue;
        if(info.type=="beacon")//if allocating beacon, must be a dedicated slot, no freq reuse (potential conflict)
          return false;
        //KILBYIIOT-6, beacon is no longer monitored after association:  
        //Tao: this is added back, since it can cause potential conflict
        if(this.schedule[slot.slot_offset][ch][sub].type=="beacon")
          return false;
        if(nodes_list.indexOf(this.schedule[slot.slot_offset][ch][sub].sender)!=-1)
          return false;
        if(nodes_list.indexOf(this.schedule[slot.slot_offset][ch][sub].receiver)!=-1)
          return false;
      }
    }
    return true;
  }
  //generate a shuffled slot list to iterate
  this.shuffle_slots=function(){
    var shuffled_slots=[];
    for(var slot=0;slot<this.slotFrameLength;++slot){
      for(var c in this.channels){
        var ch=this.channels[c];
        shuffled_slots.push({slot_offset:slot, channel_offset:ch});
      }
    }
    for (var i = shuffled_slots.length - 1; i > 0; i--) {
        var j = Math.floor(Math.random() * (i + 1));
        var temp = shuffled_slots[i];
        shuffled_slots[i] = shuffled_slots[j];
        shuffled_slots[j] = temp;
    }
    var schedule=this.schedule;
    //pass into sort(func)
    //sort the shuffled list by subslot occupation
    shuffled_slots.sort(function(a,b){
      var ca=0,cb=0;
      var a_sublist=schedule[a.slot_offset][a.channel_offset];
      var b_sublist=schedule[b.slot_offset][b.channel_offset];
      for(var sub=0;sub<SUBSLOTS;++sub){
        if(a_sublist[sub]==null){
          ++ca;
        }
        if(b_sublist[sub]==null){
          ++cb;
        }
      }
      return ca-cb;
    });
    return shuffled_slots;
  }
  // generate a slot list inside the partition.
  // flag=0: normal case, find in_partition slots
  // flag=1: assign non-optimal slots in layer 0 reserved area
  // flag=2: return huge slots list to find the needed size to 
  //         assign all its non-optimal slots back.
  this.inpartition_slots=function(flag,info){
    // console.log("Partition schedule locating: "+info.type+", layer="+info.layer)
    var inpartition_slots=[];//result slot list

    var start=0;
    var end=0;
    if(info.type=="beacon"){
      if(this.partition.broadcast!=null){
        start=this.partition.broadcast.start;
        end=this.partition.broadcast.end;
      }
    }
    if(info.type=="uplink"){
      if(this.partition.uplink!=null&&this.partition.uplink[info.layer.toString()]!=null){
        start=this.partition.uplink[info.layer.toString()].start;
        end=this.partition.uplink[info.layer.toString()].end;
      }
    }
    if(info.type=="downlink"){
      if(this.partition.downlink!=null&&this.partition.downlink[info.layer.toString()]!=null){
        start=this.partition.downlink[info.layer.toString()].start;
        end=this.partition.downlink[info.layer.toString()].end;
      }
    }
    
    // var m=Math.floor((start+end)/2);    
    // expand partition size
    if(flag==2) {
      if(info.type=="uplink") end+=30
      else start-=30
    }
    
    //sorted slot offset list, from edge
    //if first layer tricks, from center
    var partition_slot_list=[];
    for(var i=0;i<end-start;++i){
      partition_slot_list.push(0);
    }

    // flag==0: if try to find optimal slots
    if(flag==0 || flag==2) {
      // higher layers
      if(info.layer>0){
        if(info.type=="uplink") {
          // as early as possible
          for(var i=0;i<end-start;++i){
            partition_slot_list[i]=start+i;
          }
        } else {
          // as late as possible
          for(var i=0;i<end-start;i++){
            partition_slot_list[i]=end-1-i;
          }
        }
        
      }else{
        // first layer
        if(info.type=="uplink"){
          //uplink 0, as late as possible
          for(var i=0;i<end-start;++i){
            partition_slot_list[i]=end-1-i;
          }
          
        } else {
          // downlink 0, as early as possible
          for(var i=0;i<end-start;++i){
            partition_slot_list[i]=start+i;
          }
        }
      }
    } else if(flag==1){
      // flag==1: find available slots in reserved area (layer 0 other channels)
      // from edge to center
      if(info.type=="uplink"){
        //uplink 0, as late as possible
        for(var i=0;i<end-start;i++){
          partition_slot_list[i]=end-1-i;
        }
      } else {
        // downlink 0, as early as possible
        for(var i=0;i<end-start;++i){
          partition_slot_list[i]=start+i;
        }
      }
    }

    //generate slot list
    for(var i=0;i<end-start;++i){
      var slot=partition_slot_list[i];
      if(flag==0 || flag==2) {
        if(info.type=="beacon"){
          inpartition_slots.push({slot_offset:slot, channel_offset:this.channels[0]});
        }else{
          for(var c in this.channels){
            var ch=this.channels[c];
            inpartition_slots.push({slot_offset:slot, channel_offset:ch});
          }
        }
      // find available slots in reserved area (layer 0 other channels)
      } else if(flag==1) {
        for(var k=1;k<this.channels.length;k++){
          var ch=this.channels[k];
          inpartition_slots.push({slot_offset:slot, channel_offset:ch});
        }
      }
    }
    return inpartition_slots;
  }
  
  // calculate the needed size(slot range) to assign non-optimal cells
  this.calc_needed_slots=function(type,layer) {
    var slots_list = this.inpartition_slots(2,{type:type,layer:layer})
    // make a copy
    var sch_cp = JSON.parse(JSON.stringify(this.schedule));
    var used_subslot = JSON.parse(JSON.stringify(this.used_subslot));

    this.assign_slot_sim = function(cell) {
      for(var i=0;i<slots_list.length;++i){
        var slot=slots_list[i];
        var candidate = 0
        if(sch_cp[slot.slot_offset][slot.channel_offset][0]!=null) {
          // not this partition
          if(sch_cp[slot.slot_offset][slot.channel_offset][0].type!=type&&sch_cp[slot.slot_offset][slot.channel_offset][0].layer!=layer) {
            candidate = slot
          }
        } else {
          var nodes_list = [cell.sender,cell.receiver]
          // slot is empty
          candidate = slot
        }
        // not find
        if(!candidate) continue

        // check if this slot (any channel) is assigned to the members
        var pass=0
        for(var c in this.channels) {
          var ch=this.channels[c];
          if(sch_cp[candidate.slot_offset][ch][0]!=null) {
            if(nodes_list.indexOf(sch_cp[candidate.slot_offset][ch][0].sender)==-1 && nodes_list.indexOf(sch_cp[candidate.slot_offset][ch][0].receiver)==-1) {
              pass++
            }
          } else pass++
        }
        if(pass==this.channels.length) {
          sch_cp[candidate.slot_offset][candidate.channel_offset][0] = {
            type:cell.type,
            layer: cell.layer,
            sender:cell.sender,
            receiver:cell.receiver,
            is_optimal: 1,
          }
          return {slot:[candidate.slot_offset, candidate.channel_offset],cell:cell,is_optimal:1}
        }
      }
    }

    for(var j=0;j<this.used_subslot.length;j++) {
      if(!this.used_subslot[j].is_optimal && this.used_subslot[j].cell.type==type && this.used_subslot[j].cell.layer==layer) {
        var ret = this.assign_slot_sim(this.used_subslot[j].cell)
        used_subslot.push(ret)
      }
    }

    var diff
    // [start, end)
    var original_size = [this.partition[type][layer].start,this.partition[type][layer].end]
    var max = original_size[0]
    var min = original_size[1]
    for(var k=0;k<used_subslot.length;k++) {
      if(used_subslot[k].is_optimal&&used_subslot[k].cell.type==type&&used_subslot[k].cell.layer==layer) {
        if(min>=used_subslot[k].slot[0]) min=used_subslot[k].slot[0]
        if(max<=used_subslot[k].slot[0]) max=used_subslot[k].slot[0]
      }
    }
    if(type=='uplink') {
      diff = max+1-original_size[1]
    } else {
      diff = original_size[0]-min
    }
    return diff
  }

  // adjust the partition (and its slots) offset and its neighbour's size
  // offset >0 right, <0 left
  this.adjust_partition_offset=function(type,layer,offset) {
    // broadcast partition do not need adjust, for now
    if(type=="broadcast"||offset==0) return

    // adjust cells offset
    // adjust cells in this.schedule
    for(var c in this.channels) {
      var ch = this.channels[c]
      // offset>0, rear to front
      if(offset>0) {
        for(var slot=this.partition[type][layer].end;slot>=this.partition[type][layer].start;slot--) {
          for(var sub=0;sub<SUBSLOTS;++sub) {
            if(this.schedule[slot][ch][sub]!=null) {
              this.schedule[slot+offset][ch][sub] = this.schedule[slot][ch][sub]
              this.schedule[slot][ch][sub] = null
            }
          }
        }
      } else {
        // offset<0, front to rear
        for(var slot=this.partition[type][layer].start;slot<this.partition[type][layer].end;slot++) {
          for(var sub=0;sub<SUBSLOTS;++sub) {
            if(this.schedule[slot][ch][sub]!=null) {
              this.schedule[slot+offset][ch][sub] = this.schedule[slot][ch][sub]
              this.schedule[slot][ch][sub] = null
            }
          }
        }
      }
    }
    
    var count = 0
    // adjust cells in this.used_subslot  
    for(var i=0;i<this.used_subslot.length;i++) {
      if(this.used_subslot[i].cell.type==type && this.used_subslot[i].cell.layer==layer && this.used_subslot[i].is_optimal) {
        this.used_subslot[i].slot[0]+=offset
        count++
      }
    }
    
    // adjust partition offset
    this.partition[type][layer].start += offset
    this.partition[type][layer].end += offset
    
    if(layer==1) {
      if(type=="uplink"&&offset<0) this.partition[type][0].start+=offset
      if(type=="downlink"&&offset>0) this.partition[type][0].end+=offset
    }

    return count
  }

  // adjust the partition and its neighbour's size
  // side: 'left' or 'right'
  // offset >0 expand, <0 shrink
  // U0 shall not adjust right side, D0 shall not adjust left side
  this.adjust_partition_size=function(type,layer,side,offset) {
    // broadcast partition do not need adjust, for now
    if(type=="broadcast") return
    if(side=="left") {
      this.partition[type][layer].start -= offset
    }
    if(side=="right") {
      this.partition[type][layer].end += offset
    }
    if(layer==1) {
      if(type=="uplink"&&side=="right") this.partition['uplink'][0].start = this.partition['uplink'][1].end
      if(type=="downlink"&&side=="left") this.partition['downlink'][0].end = this.partition['downlink'][1].start
    }
  }

  this.adjustment_summary = {}
  this.adjust=function(type, layer) {
    if(layer==0) return
    sides = ['right','left']
    var side = (type=="uplink")?0:1
    var sign = (type=="uplink")?1:-1
    // console.log("[*] adjusting",type,layer,'...')

    var gap = 1
    var needed_size = this.calc_needed_slots(type,layer) + gap // leave some space
    // console.log('   ',type,layer,"needs", needed_size)

    // expand, low layers first
    if(needed_size>0) {
      for(var l=1;l<layer;l++) {
        var count = this.adjust_partition_offset(type,l,needed_size*sign)
        var name = type[0]+l
        if(this.adjustment_summary[name]==null) this.adjustment_summary[name] = {affected_cells: count, offset:0}
        this.adjustment_summary[name].offset += needed_size*sign
      }
    // shrink, high layers first
    } else {
      for(var l=layer-1;l>0;l--) {
        var count = this.adjust_partition_offset(type,l,needed_size*sign)
        var name = type[0]+l
        if(this.adjustment_summary[name]==null) this.adjustment_summary[name] = {affected_cells: count, offset:0}
        this.adjustment_summary[name].offset += needed_size*sign
      }
    }
    // console.log("    neighbours move to the",sides[side],"by", needed_size)
    this.adjust_partition_size(type, layer, sides[side], needed_size)
    // console.log("   ",type,layer , "expands to the", sides[side], "by", needed_size)

    this.adjust(type, layer-1)
  }

  this.dynamic_partition_adjustment=function() {
    // highest layer of non-optmial cells
    var highest_layer = Object.keys(this.partition['uplink']).length-1
    
    // exec twice, 1 for non-optimals, 1 for gap adjustment
    this.adjust('uplink',highest_layer); this.adjust('downlink',highest_layer)
    this.adjust('uplink',highest_layer); this.adjust('downlink',highest_layer)

    // console.log('Partitions offset adjustment summary:',this.adjustment_summary)
    
    // make a real/deep copy! add or remove will change sch.used_subslot length
    var used_subslot = JSON.parse(JSON.stringify(this.used_subslot));
    var cnt = 0
    for(var j=0;j<used_subslot.length;j++) {
      if(!used_subslot[j].is_optimal) {
        var old = used_subslot[j]
        // console.log("adjusting",old)
        var ret = this.find_empty_subslot([old.cell.sender, old.cell.receiver],1,{type:old.cell.type,layer:old.cell.layer})
        // console.log("to new position",ret)
        this.add_subslot(ret.slot, ret.subslot, {type:old.cell.type,layer:old.cell.layer,sender:old.cell.sender,receiver:old.cell.receiver}, 1);
        this.remove_subslot({slot_offset:old.slot[0],channel_offset:old.slot[1]}, {offset:old.subslot[0], period:old.subslot[1]})
        cnt++
      }
    }
    // console.log("Put", cnt, "non-optimal cells back")
  }

  //3-d searcher
  this.find_empty_subslot=function(nodes_list, period, info){
    var slots_list;

    //This part is the partitioned scheduler
    if((algorithm == PART || algorithm == PARTPLUS ) && info!=null){
      slots_list=this.inpartition_slots(0,info);
      for(var i=0;i<slots_list.length;++i){
        var slot=slots_list[i];

        for(var offset=0;offset<period;++offset){
          if(this.available_subslot(nodes_list,slot,{period:period,offset:offset},info)){
            ret = {row:0,slot:slot,subslot:{offset:offset,period:period},is_optimal:1}
            // console.log("Empty subslot found:",ret)
            return(ret);
          }
        }
      }
      // console.log("No emplty slot in partition, non-optimal shuffled scheduling!");
      this.nonOptimalCount++;
    }

    // assign a random slot
    // slots_list=this.shuffle_slots();
    // assign in reserved area
    slots_list = this.inpartition_slots(1, {type:info.type, layer: 0});
    for(var i=0;i<slots_list.length;++i){
      var slot=slots_list[i];
      for(var offset=0;offset<period;++offset){
        if(this.available_subslot(nodes_list,slot,{period:period,offset:offset},info)){
          var ret = {row:0,slot:slot,subslot:{offset:offset,period:period}, is_optimal:0}
          // console.log("find an alternative slot:",ret);
          return(ret);
        }
      }
    }
    
    console.log("No emplty slot found");
    this.isFull=true;
    return null;
  }

  this.remove_node=function(node){
    for(var slot=0;slot<this.slotFrameLength;++slot){
      for(var c in this.channels){
        var ch=this.channels[c];
        for(var sub=0;sub<SUBSLOTS;++sub){
          if(this.schedule[slot][ch][sub]!=null){
            if( 
              this.schedule[slot][ch][sub].sender==node ||
              this.schedule[slot][ch][sub].receiver==node
            ){
              // console.log("schedule["+slot+"]["+ch+"]["+sub+"] cleaned");
              this.schedule[slot][ch][sub]=null;
            }
          }
        }
      }
    }
    for(var i=0;i<this.used_subslot.length;i++) {
      if(this.used_subslot[i].cell.sender==node||this.used_subslot[i].cell.receiver==node) {
        this.used_subslot.splice(i,1)
        // array length will change
        i--
      }
    }
  }

  this.count_slot=function(sender_list, receiver_list){
  //this function is to count the used slot. Different Channels count as 1
    var res=0;
    for(var slot=0;slot<this.slotFrameLength;++slot){
      var used=false;
      for(var c in this.channels){
        var ch=this.channels[c];
        for(var sub=0;sub<SUBSLOTS;++sub){
          if(this.schedule[slot][ch][sub]!=null){
            if( 
              sender_list.indexOf(+this.schedule[slot][ch][sub].sender)!=-1 &&
              receiver_list.indexOf(+this.schedule[slot][ch][sub].receiver)!=-1
            ){
              used=true;
            }
          }
        }
      }
      if(used)
        ++res;
    }
    return res;
    
  }
  this.visualize=function(n){
    var total=0;
    var tbody="";
    tbody+="<tr>";
    tbody+="<td>Slot</td>";
    for(var slot=0;slot<this.slotFrameLength;++slot){
      tbody+="<td>";
      tbody+=slot;
      tbody+="</td>";
    }
    tbody+="</tr>\n";
    for(var c in this.channels){
      var ch=this.channels[c];
      tbody+="<tr>";
      tbody+="<td>";
      tbody+="CH"+ch;
      tbody+="</td>";
      for(var slot=0;slot<this.slotFrameLength;++slot){
        tbody+="<td>";

        tbody+="<table>";
        var last={sender:0,receiver:0,type:""};

        for(var sub=0;sub<SUBSLOTS;++sub){
          if(this.schedule[slot][ch][sub]!=null){
            var cell=this.schedule[slot][ch][sub];
            if(n==0||cell.sender==n||cell.receiver==n){
              //supprese same output
              if(!(cell.sender == last.sender 
                && cell.receiver == last.receiver
                && cell.type == last.type)
              ){
                tbody+="<tr><td>";
                tbody+=(cell.sender+"->"+(cell.receiver!=0xffff?cell.receiver:"BCAST"));
                tbody+="<br>";
                tbody+=cell.type;
                if(!cell.  is_optimal) {
                  tbody+=" (non-optimal!)"
                }
                last={sender:cell.sender,receiver:cell.receiver,type:cell.type};
                tbody+="</td></tr>"
                // link count in 1 slotframe
                total++
              }
              // 16*tx+16*rx+1*beacon, link count in 16 slotframes
              // total++;
            }
          }
        }
        tbody+="</table>";

        tbody+="</td>";
      }
      tbody+="</tr>\n";
    }
    var head="<p>Total: "+total+" Links</p>" + "<p>isFull: "+this.isFull.toString()+"</p><p>nonOptimalCount: "+this.nonOptimalCount.toString()+"</p>";
    return head+"<table>"+tbody+"</table>";
  }
  
  this.periodOffsetToSubslot = function(periodOffset, period)
  {
     return periodOffset*((SUBSLOTS/period) % SUBSLOTS);
  }
}

module.exports={
  create_scheduler:create_scheduler,
  SUBSLOTS : SUBSLOTS
};


