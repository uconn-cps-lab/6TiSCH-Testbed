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


const RESERVED = 4; //Number of reserved
const SUBSLOTS = 8;
const ROWS = 4;
const ALGORITHM = "partition"
const partition_config = require("./partition.json");

function partition_init(sf) {
  //Beacon reserved version
  var u_d = [Math.floor(sf - partition_config.beacon - RESERVED -partition_config.control ) / 2, 
    Math.floor(sf - partition_config.beacon - RESERVED - partition_config.control) / 2];
  // var u_d = [sf-partition_config.beacon,sf-partition_config.beacon]
  // partition_scale(u_d, sf-RESERVED-partition_config.beacon);
  var uplink = partition_config.uplink.slice(); 
  var downlink = partition_config.downlink.slice();

  var partition = {
    broadcast: {},
    control:{},
  }
  for (var r = 0; r < ROWS; r++) {
    partition[r] = { uplink: {}, downlink: {} }
  }

  var channel_per_row = Math.floor(16/ROWS)
  var slot_cursor = []
  var uplink_row = []
  var downlink_row = []
  var last_uplink_row_boundary = 0
  var last_downlink_row_boundary = 0

  partition.control = { start: RESERVED, end: RESERVED+partition_config.control };

  for(var r=0; r<ROWS;r++) {
    slot_cursor[r] = partition.control.end
    uplink_row[r] = partition_scale(uplink, u_d[0]-last_uplink_row_boundary)
    // uplink_row[r] = uplink
    downlink_row[r] = partition_scale(downlink, u_d[1]-last_downlink_row_boundary);
    last_uplink_row_boundary += uplink_row[r][0]
    last_downlink_row_boundary += downlink_row[r][0]
  }

  //now we have everything scaled by slotframe length
  //and start do the partitioning
  
  // uplink
  for (var u = uplink.length - 1; u >= 0; --u) {
    for (var r=0;r<ROWS;r++) {
      var tmp = 0
      // if(r==ROWS-1) tmp=1
      partition[r].uplink[u] = {start: slot_cursor[r], end: slot_cursor[r]+uplink_row[r][u], channels:[r*channel_per_row,(r+1)*channel_per_row+tmp]}
      slot_cursor[r] += uplink_row[r][u]
    }
  }

  // Beacon
  partition.broadcast = { start: slot_cursor[0], end: slot_cursor[0] + partition_config.beacon };
  
  for(var r=0;r<ROWS;r++) {
    slot_cursor[r] = sf
  }

  // Downlink
  for (var d = downlink.length - 1; d >= 0; --d) {
    for (var r=0;r<ROWS;r++) {
      var tmp = 0
      // if(r==ROWS-1) tmp=1
      partition[r].downlink[d] = {start: slot_cursor[r]-downlink_row[r][d], end: slot_cursor[r], channels:[r*channel_per_row,(r+1)*channel_per_row+tmp]}
      slot_cursor[r] -= downlink_row[r][d]
    }
  }

  console.log("patition:", partition);
  return partition;
}

function partition_scale(raw_list, size) {
  var list = JSON.parse(JSON.stringify(raw_list));
  //scale the list to the size (sum(list)==size)
  var total = 0;
  for (var i = 0; i < list.length; ++i) {
    total += list[i];
  }
  for (var i = 0; i < list.length; ++i) {
    list[i] = list[i] * size / total;
  }
  //Now we need to arrange them to integers. We cannot directly floor or round or ceil
  //These will cause the sum!=size
  //we do the following 3 for's to round the boundary to closest integers
  for (var i = 1; i < list.length; ++i) {
    list[i] += list[i - 1];
  }
  for (var i = 0; i < list.length; ++i) {
    list[i] = Math.floor(list[i] + 0.5);
  }
  for (var i = list.length - 1; i > 0; --i) {
    list[i] -= list[i - 1];
  }

  return list;
}

function create_scheduler(sf, ch) {
  /*
  Slot = {slot_offset, channel}
  Subslot = {period, offset}
  Cell = {type, layer, row, sender, receiver}
  
  used_subslot = {slot: [slot_offset, ch_offset], subslot: [periord, offset], cell: cell, is_optimal:1}
  */
  if (!(this instanceof create_scheduler)) {
    return new create_scheduler(sf, ch);
  }

  console.log("create_scheduler("+sf+","+ch+")", ALGORITHM);
  this.slotFrameLength = sf;
  this.channels = ch;
  this.algorithm = ALGORITHM
  this.rows = ROWS
  this.schedule = new Array(sf);
  // { parent: [children] }, mainly for count subtree size
  this.topology = { 0: [] }
  // mainly for send cells to cloud
  this.used_subslot = []
  this.isFull = false;
  this.nonOptimalCount = 0;
  for (var i = 0; i < sf; ++i) {
    this.schedule[i] = new Array(16);
  }
  for (var c in this.channels) {
    var ch = this.channels[c];
    for (var slot = 0; slot < this.slotFrameLength; ++slot) {
      this.schedule[slot][ch] = new Array(SUBSLOTS)
    }
  }

  //initialize partition
  this.partition = partition_init(sf);
  console.log(this.partition)
  this.channelRows = {}
  for(var r=0;r<ROWS;r++) {
    var channels = this.partition[r].uplink[0].channels
    this.channelRows[r] = []
    for(var c=channels[0];c<channels[1];c++) {
      this.channelRows[r].push(c+1)
    }
  }
  console.log(this.channelRows)
  // this.channelRows = { 0: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10], 1: [11, 12, 13, 14], 2: [15, 16] }

  this.setTopology = function (topo) {
    this.topo = topo
  }

  this.add_slot = function (slot, cell) {
    this.add_subslot(slot, { offset: 0, period: 1 }, cell);
  }

  this.add_subslot = function (slot, subslot, cell, is_optimal) {
    var sub_start = subslot.offset * SUBSLOTS / subslot.period;
    var sub_end = (subslot.offset + 1) * SUBSLOTS / subslot.period;

    for (var sub = sub_start; sub < sub_end; ++sub) {
      this.schedule[slot.slot_offset][slot.channel_offset][sub] = {
        type: cell.type,
        layer: cell.layer,
        sender: cell.sender,
        receiver: cell.receiver,
        option: cell.option
      }
    }

    this.used_subslot.push({ slot: [slot.slot_offset, slot.channel_offset], subslot: [subslot.offset, subslot.period], cell: cell, is_optimal: is_optimal })

    if (cell.type == "uplink") {
      if (this.topology[cell.receiver] != null) {
        if (this.topology[cell.receiver].indexOf(cell.sender) == -1) {
          this.topology[cell.receiver].push(cell.sender)
        }
      }
      else this.topology[cell.receiver] = [cell.sender]
    }

  }

  this.remove_slot = function (slot) {
    this.remove_subslot(slot, { offset: 0, period: 1 });
  }

  this.remove_subslot = function (slot, subslot) {
    var sub_start = subslot.offset * SUBSLOTS / subslot.period;
    var sub_end = (subslot.offset + 1) * SUBSLOTS / subslot.period;
    for (var sub = sub_start; sub < sub_end; ++sub) {
      this.schedule[slot.slot_offset][slot.channel_offset][sub] = null;
    }

    for (var i = 0; i < this.used_subslot.length; i++) {
      if (this.used_subslot[i].slot[0] == slot.slot_offset &&
        this.used_subslot[i].slot[1] == slot.channel_offset &&
        this.used_subslot[i].subslot[0] == subslot.offset &&
        this.used_subslot[i].subslot[1] == subslot.period) {
        this.used_subslot.splice(i, 1)
        i--
      }
    }
  }

  // remove used subslot by sender, receiver, type
  this.remove_usedsubslot = function (sender, receiver, type) {
    for (var i = 0; i < this.used_subslot.length; i++) {
      if (this.used_subslot[i].cell.sender == sender &&
        this.used_subslot[i].cell.receiver == receiver &&
        this.used_subslot[i].cell.type == type) {
        this.used_subslot.splice(i, 1)
        i--
      }
    }
  }

  //3-d filter
  //flag=1: check order
  this.available_subslot = function (nodes_list, slot, subslot, info, flag) {
    if (slot.slot_offset < RESERVED) return false;
    // this.partition.broadcast.start = 125
    // this.partition.broadcast.end = 127
    //if is beacon, we want it always the first channel in the list;
    //it actually doesn't matter since hardware-wise it's hardcoded to beacon channel
    //but to make it consistant in scheduler table...
    if (info.type == "beacon" && slot.channel_offset != this.channels[0]) return false;

    //Beacon reserved version: broadcast partition can only be utilized by beacon
    if (this.partition.broadcast != null) {
      var start = this.partition.broadcast.start;
      var end = this.partition.broadcast.end;
      if (info.type == "beacon") {
        if (slot.slot_offset < start || slot.slot_offset >= end) return false;
      } else {
        if (slot.slot_offset >= start && slot.slot_offset < end) return false;
      }
    }

    //check if this slot-channel is assigned
    var sub_start = subslot.offset * SUBSLOTS / subslot.period;
    var sub_end = (subslot.offset + 1) * SUBSLOTS / subslot.period;
    for (var sub = sub_start; sub < sub_end; ++sub) {
      if (this.schedule[slot.slot_offset][slot.channel_offset][sub] != null)
        return false;
    }

    //check if this slot (any channel) is assigned to beacon
    //or is assigned to the members
    for (var c in this.channels) {
      var ch = this.channels[c];
      for (var sub = sub_start; sub < sub_end; ++sub) {
        if (this.schedule[slot.slot_offset][ch][sub] == null)
          continue;
        // if(info.type=="beacon")//if allocating beacon, must be a dedicated slot, no freq reuse (potential conflict)
        //   return false;
        //KILBYIIOT-6, beacon is no longer monitored after association:  
        //Tao: this is added back, since it can cause potential conflict
        if (this.schedule[slot.slot_offset][ch][sub].type == "beacon") return false;
        if (nodes_list.indexOf(this.schedule[slot.slot_offset][ch][sub].sender) != -1) return false;
        if (nodes_list.indexOf(this.schedule[slot.slot_offset][ch][sub].receiver) != -1) return false;
      }
    }


    if (flag && info.layer > 0) {
      if (info.type != "beacon") {
        var parent = 0
        if (info.type == "uplink") parent = nodes_list[1]
        else parent = nodes_list[0]

        var parent_slot = this.find_cell(parent, info.type, info.option)
        if (info.type == "uplink") {
          if (slot.slot_offset > parent_slot.slot[0]) return false
        } else {
          if (slot.slot_offset < parent_slot.slot[0]) return false
        }
      }
    }
    return true;
  }

  // generate a slot list inside the partition.
  // flag=0: normal case, find in_partition slots
  // flag=1: assign non-optimal slots in layer 0 reserved area
  // flag=2: return huge slots list to find the needed size to 
  //         assign all its non-optimal slots back.
  this.inpartition_slots = function (flag, info, row) {
    // console.log("Partition schedule locating: "+info.type+", layer="+info.layer)
    var inpartition_slots = [];//result slot list

    var start = 0;
    var end = 0;
    if (info.type == "beacon") {
      if (this.partition.broadcast != null) {
        start = this.partition.broadcast.start;
        end = this.partition.broadcast.end;
      }
    }
    if (info.type == "uplink") {
      if (this.partition[row].uplink != null && this.partition[row].uplink[info.layer.toString()] != null) {
        start = this.partition[row].uplink[info.layer.toString()].start;
        end = this.partition[row].uplink[info.layer.toString()].end;
      }
    }
    if (info.type == "downlink") {
      if (this.partition[row].downlink != null && this.partition[row].downlink[info.layer.toString()] != null) {
        start = this.partition[row].downlink[info.layer.toString()].start;
        end = this.partition[row].downlink[info.layer.toString()].end;
      }
    }

    // var m=Math.floor((start+end)/2);    
    // expand partition size
    if (flag == 2) {
      if (info.type == "uplink") end += 30
      else start -= 30
    }

    //sorted slot offset list, from edge
    //if first layer tricks, from center
    var partition_slot_list = [];
    for (var i = 0; i < end - start; ++i) {
      partition_slot_list.push(0);
    }

    // flag==0: if try to find optimal slots
    if (flag == 0 || flag == 2) {
      if (info.type == "uplink") {
        //uplink 0, as late as possible
        for (var i = 0; i < end - start; i++) {
          partition_slot_list[i] = end - 1 - i;
        }
      } else {
        // downlink 0, as early as possible
        for (var i = 0; i < end - start; ++i) {
          partition_slot_list[i] = start + i;
        }
      }


    } else if (flag == 1) {
      // flag==1: find available slots in reserved area (layer 0 other channels)
      // from edge to center
      if (info.type == "uplink") {
        //uplink 0, as late as possible
        for (var i = 0; i < end - start; i++) {
          partition_slot_list[i] = end - 1 - i;
        }
      } else {
        // downlink 0, as early as possible
        for (var i = 0; i < end - start; ++i) {
          partition_slot_list[i] = start + i;
        }
      }
    }

    //generate slot list
    for (var i = 0; i < end - start; ++i) {
      var slot = partition_slot_list[i];
      if (flag == 0 || flag == 2) {
        if (info.type == "beacon") {
          inpartition_slots.push({ slot_offset: slot, channel_offset: this.channels[0] });
        } else {
          for (var c in this.channelRows[row]) {
            var ch = this.channelRows[row][c]
            inpartition_slots.push({ slot_offset: slot, channel_offset: ch });
          }
        }
        // find available slots in reserved area (layer 0 other channels)
      } else if (flag == 1) {
        for (var k = 1; k < this.channels.length - 1; k++) {
          var ch = this.channels[k];
          inpartition_slots.push({ slot_offset: slot, channel_offset: ch });
        }
      }
    }
    return inpartition_slots;
  }

  // calculate the needed size(slot range) to assign non-optimal cells
  this.calc_needed_slots = function (row, type, layer) {
    var slots_list = this.inpartition_slots(2, { type: type, layer: layer }, row)
    // make a copy
    var sch_cp = JSON.parse(JSON.stringify(this.schedule));
    var used_subslot = JSON.parse(JSON.stringify(this.used_subslot));

    this.assign_slot_sim = function (cell) {
      var ret = {}
      for (var i = 0; i < slots_list.length; ++i) {
        var slot = slots_list[i];
        var candidate = 0
        if (sch_cp[slot.slot_offset][slot.channel_offset][0] != null) {
          // not this partition
          if (sch_cp[slot.slot_offset][slot.channel_offset][0].type != type && sch_cp[slot.slot_offset][slot.channel_offset][0].layer != layer) {
            candidate = slot
          }
        } else {
          var nodes_list = [cell.sender, cell.receiver]
          // slot is empty
          candidate = slot
        }
        // not find
        if (!candidate) continue

        // check order
        var parent = 0
        if (cell.type == "uplink") parent = cell.receiver
        else parent = cell.sender
        var parent_slot = this.find_cell(parent, cell.type, cell.layer - 1)
        if (cell.sendertype == "uplink") {
          if (slot.slot_offset > parent_slot) continue
        } else {
          if (slot.slot_offset < parent_slot && Math.abs(slot.slot_offset - parent_slot) < 60) continue
        }

        // check if this slot (any channel) is assigned to the members
        var pass = 0
        for (var c in this.channels) {
          var ch = this.channels[c];
          if (sch_cp[candidate.slot_offset][ch][0] != null) {
            if (nodes_list.indexOf(sch_cp[candidate.slot_offset][ch][0].sender) == -1 && nodes_list.indexOf(sch_cp[candidate.slot_offset][ch][0].receiver) == -1) {
              pass++
            }
          } else pass++
        }
        if (pass == this.channels.length) {
          sch_cp[candidate.slot_offset][candidate.channel_offset][0] = {
            type: cell.type,
            layer: cell.layer,
            row: cell.row,
            sender: cell.sender,
            receiver: cell.receiver,
            is_optimal: 1,
          }
          ret = { slot: [candidate.slot_offset, candidate.channel_offset], cell: cell, is_optimal: 1 }
          break
        }
      }
      return ret
    }

    for (var j = 0; j < this.used_subslot.length; j++) {
      if (!this.used_subslot[j].is_optimal && this.used_subslot[j].cell.type == type && this.used_subslot[j].cell.row == row &&
        this.used_subslot[j].cell.layer == layer) {
        var ret = this.assign_slot_sim(this.used_subslot[j].cell)
        if (ret != null)
          used_subslot.push(ret)
      }
    }

    var diff = 0
    // [start, end)
    var original_size = [this.partition[row][type][layer].start, this.partition[row][type][layer].end]
    var max = original_size[0]
    var min = original_size[1]
    for (var k = 0; k < used_subslot.length; k++) {
      if (used_subslot[k].is_optimal && used_subslot[k].cell.type == type && used_subslot[k].cell.row == row && used_subslot[k].cell.layer == layer) {
        if (min >= used_subslot[k].slot[0]) min = used_subslot[k].slot[0]
        if (max <= used_subslot[k].slot[0]) max = used_subslot[k].slot[0]
      }
    }
    if (row == 0) {
      diff = original_size[0] - min
    } else {
      diff = max + 1 - original_size[1]
    }
    return diff
  }

  // adjust the partition (and its slots) offset and its neighbour's size
  // offset >0 right, <0 left
  this.adjust_partition_offset = function (row, type, layer, offset) {
    // broadcast partition do not need adjust, for now
    if (type == "broadcast" || offset == 0) return

    // adjust cells offset
    // adjust cells in this.schedule

    for (var c in this.channelRows[row]) {
      var ch = this.channelRows[row][c]
      // offset>0, rear to front
      if (offset > 0) {
        for (var slot = this.partition[row][type][layer].end; slot >= this.partition[row][type][layer].start; slot--) {
          for (var sub = 0; sub < SUBSLOTS; ++sub) {
            if (this.schedule[slot][ch][sub] != null) {
              this.schedule[slot + offset][ch][sub] = this.schedule[slot][ch][sub]
              this.schedule[slot][ch][sub] = null
            }
          }
        }
      } else {
        // offset<0, front to rear
        for (var slot = this.partition[row][type][layer].start; slot < this.partition[row][type][layer].end; slot++) {
          for (var sub = 0; sub < SUBSLOTS; ++sub) {
            if (this.schedule[slot][ch][sub] != null) {
              this.schedule[slot + offset][ch][sub] = this.schedule[slot][ch][sub]
              this.schedule[slot][ch][sub] = null
            }
          }
        }
      }
    }

    var count = 0
    // adjust cells in this.used_subslot  
    for (var i = 0; i < this.used_subslot.length; i++) {
      if (this.used_subslot[i].cell.type == type && this.used_subslot[i].cell.row == row &&
        this.used_subslot[i].cell.layer == layer && this.used_subslot[i].is_optimal) {
        this.used_subslot[i].slot[0] += offset
        count++
      }
    }

    // adjust partition offset
    if (type == "uplink") {
      this.partition[row][type][layer - 1].start += offset
      this.partition[row][type][layer].end += offset
    } else {
      this.partition[row][type][layer].end += offset
      this.partition[row][type][layer].start += offset

      // if(layer==this.partition.downlink.length-1) 
      // this.adjust_partition_offset(0,'downlink',0,offset)

    }
    return count
  }

  // adjust the partition and its neighbour's size
  // side: 'left' or 'right'
  // offset >0 expand, <0 shrink
  // U0 shall not adjust right side, D0 shall not adjust left side
  this.adjust_partition_size = function (row, type, layer, side, offset) {
    // broadcast partition do not need adjust, for now
    if (type == "broadcast") return
    if (side == "left") {
      this.partition[row][type][layer].start -= offset
    }
    if (side == "right") {
      this.partition[row][type][layer].end += offset
    }
    if (layer == 1) {
      if (type == "uplink" && side == "right") this.partition[row]['uplink'][0].start = this.partition[row]['uplink'][1].end
      if (type == "downlink" && side == "left") {

        this.partition[row]['downlink'][0].end = this.partition[row]['downlink'][1].start
      }
    }
  }

  this.get_gap = function (row, type, layer) {
    // if(layer==1) return 10
    // if(layer==2) return 7
    // if(layer==3) return 3 
    // if(layer==4) return 3
    return 2
  }

  this.adjustment_summary = { '0': {}, '1': {} }

  // adjust partition boundary
  this.adjust = function (row, type, layer) {
    if (layer == 0) return
    sides = ['right', 'left']
    var side = (type == "uplink") ? 0 : 1
    var sign = (type == "uplink") ? 1 : -1
    // console.log("[*] adjusting row",row,type,layer,'...')
    var gap = this.get_gap(row, type, layer)
    var needed_size = this.calc_needed_slots(row, type, layer) + gap // leave some space
    // console.log("    needs", needed_size, "slots")
    // expand, low layers first
    if (needed_size > 0) {
      for (var l = 1; l < layer + (1 - side); l++) {
        var count = this.adjust_partition_offset(row, type, l, needed_size * sign)
        var name = type[0] + l
        if (this.adjustment_summary[row][name] == null) this.adjustment_summary[row][name] = { affected_cells: count, offset: 0 }
        this.adjustment_summary[row][name].offset += needed_size * sign
      }
      // shrink, high layers first
    } else {
      for (var l = layer - side; l > 0; l--) {
        var count = this.adjust_partition_offset(row, type, l, needed_size * sign)
        var name = type[0] + l
        if (this.adjustment_summary[row][name] == null) this.adjustment_summary[row][name] = { affected_cells: count, offset: 0 }
        this.adjustment_summary[row][name].offset += needed_size * sign
      }
    }
    // console.log("    neighbours move to the",sides[side],"by", needed_size)
    if (type == "downlink") this.adjust_partition_size(row, type, layer, sides[side], needed_size)

    // console.log("   ",type,layer , "expands to the", sides[side], "by", needed_size)

    this.adjust(row, type, layer - 1)
  }

  // find the first(uplink)/last(downlink) used slot of the node
  this.find_cell = function (node, type, option) {
    if(option) {
      for (var i = 0; i < this.used_subslot.length; i++) {
        if (type == "uplink") {
          if (this.used_subslot[i].cell.sender == node && this.used_subslot[i].cell.type == type && this.used_subslot[i].cell.option == option) {
            return this.used_subslot[i]
          }
        } else if (type == "downlink") {
          if (this.used_subslot[i].cell.receiver == node && this.used_subslot[i].cell.type == type && this.used_subslot[i].cell.option == option) {
            return this.used_subslot[i]
          }
        }
      }

    } else {
      for (var i = 0; i < this.used_subslot.length; i++) {
        if (type == "uplink") {
          if (this.used_subslot[i].cell.sender == node && this.used_subslot[i].cell.type == type) {
            return this.used_subslot[i]
          }
        } else if (type == "downlink") {
          if (this.used_subslot[i].cell.receiver == node && this.used_subslot[i].cell.type == type) {
            return this.used_subslot[i]
          }
        }
      }
    }
    
    console.log("cannot find this cell", node, type)
  }

  // get subtree size (get children and children's children number)
  this.get_subtree_size = function (node) {
    if (this.topology[node] == null) return 0
    var cnt = this.topology[node].length
    for (var i = 0; i < this.topology[node].length; i++) {
      cnt += this.get_subtree_size(this.topology[node][i])
    }
    return cnt
  }

  // get conflict slots, same sender/receiver in one slot
  // call it after adjust partition offset
  this.get_conflict_slots = function () {
    var ret = {}
    for (var i = this.partition.broadcast.end; i < 69; i++) {
      var tmp = {}
      var ch = []
      for (var j = 1; j < 17; j++) {
        if (this.schedule[i][j][0] != null) {
          if (tmp[this.schedule[i][j][0].sender] == null) {
            tmp[this.schedule[i][j][0].sender] = j
          } else {
            ch.push(tmp[this.schedule[i][j][0].sender], j)
          }
          if (tmp[this.schedule[i][j][0].receiver] == null) {
            tmp[this.schedule[i][j][0].receiver] = j
          } else {
            ch.push(tmp[this.schedule[i][j][0].receiver], j)
          }
        }
      }
      if (ch.length > 0) ret[i] = Array.from(new Set(ch))
    }
    return ret
  }


  // get idle slots number of one partition
  this.get_idle_slots = function (row, type, layer) {
    for (var i = this.partition[row][type][layer].start; i < this.partition[row][type][layer].end; i++) {
      for (var c in this.channelRows[row]) {
        // find the edge
        var ch = this.channelRows[row][c]
        if (this.schedule[i][ch][0] != null) {
          return i - this.partition[row][type][layer].start
        }
      }
    }
    return this.partition[row][type][layer].end - this.partition[row][type][layer].start
  }

  // get idle slots number of all partition
  this.get_idles_all = function () {
    var list = []
    for (var l = 1; l < Object.keys(this.partition[0]['uplink']).length; l++) {
      list[l] = this.get_idle_slots(l)
    }
    return list
  }

  // adjust neighbor's boundary to assign more slots for this partition
  this.inter_partition_adjustment = function (type, layer) {

  }

  // get needed slots to realign non-optimal cells
  // clear row 3 neighbors and reschedule
  this.get_needed_slots = function (type, layer) {
    var schedule_backup = JSON.parse(JSON.stringify(this.schedule))
    var used_subslot_backup = JSON.parse(JSON.stringify(this.used_subslot))

    for (var i = 0; i < this.used_subslot.length; i++) {
      if (!this.used_subslot[i].is_optimal && this.used_subslot[i].cell.type == type && this.used_subslot[i].cell.layer == layer) {

      }
    }
  }

  // adjust cells of within partition (all rows)
  this.intra_partition_adjustment = function (type, layer) {
    console.log("adjusting " + type + " layer " + layer + "...")
    var edits = 0
    edits += this.minimize_used_slots(type, layer)
    edits += this.adjust_subtree_distribution(type, layer)
    return edits
  }

  // get one partition cell list with subtree size
  this.get_subtree_size_list = function (type, layer) {
    var subtree_sizes = []
    for (var i = 0; i < this.used_subslot.length; i++) {
      if (this.used_subslot[i].cell.layer == layer && this.used_subslot[i].cell.type == type) {
        subtree_sizes.push({
          slot: this.used_subslot[i].slot[0],
          sender: this.used_subslot[i].cell.sender,
          size: this.get_subtree_size(this.used_subslot[i].cell.sender),
          row: this.used_subslot[i].cell.row,
          receiver: this.used_subslot[i].cell.receiver,
          ch: this.used_subslot[i].slot[1],
        })
      }
    }
    subtree_sizes.sort((a, b) => (a.slot > b.slot) ? 1 : -1)
    return subtree_sizes
  }

  // move cells with larger subtree size to row 0 and sort row 1/2
  // cells in row 0 don't need to be adjust
  this.adjust_subtree_distribution = function (type, layer) {
    this.total_edits = 0
    var subtree_sizes = this.get_subtree_size_list(type, layer)
    if (layer == 0) {
      var dividerSlot = this.partition[1][type][layer].end
      var dividerSlotIndex = 0
      for (var ii = 0; ii < subtree_sizes.length; ii++) {
        if (subtree_sizes[ii].slot == dividerSlot) {
          dividerSlotIndex = ii
          break
        }
      }
      var copy = JSON.parse(JSON.stringify(subtree_sizes))
      copy.sort((a, b) => (a.size > b.size) ? 1 : -1)

      // max subtree size in other rows (row 1,2...)
      var maxSubtreeSize = copy[dividerSlotIndex].size

      // move some cell with larger subtree size to row 0
      for (var j = 0; j < dividerSlotIndex; j++) {
        if (subtree_sizes[j].size > maxSubtreeSize) {
          // swap with a smaller one in row 0
          for (k = dividerSlotIndex; k < subtree_sizes.length; k++) {
            if (subtree_sizes[k].size <= maxSubtreeSize) {
              var tmp = JSON.parse(JSON.stringify(subtree_sizes[k]))
              var tmp2 = JSON.parse(JSON.stringify(subtree_sizes[j]))
              subtree_sizes[k].sender = tmp2.sender
              subtree_sizes[k].receiver = tmp2.receiver
              subtree_sizes[k].size = tmp2.size
              subtree_sizes[j].sender = tmp.sender
              subtree_sizes[j].receiver = tmp.receiver
              subtree_sizes[j].size = tmp.size
              break
            }
          }
        }
      }

      // cycle sort cells in other rows (row 1,2...)
      for (var cycleStart = 0; cycleStart < dividerSlotIndex; cycleStart++) {
        var item = JSON.parse(JSON.stringify(subtree_sizes[cycleStart]))
        var pos = cycleStart

        // find the right index
        for (var i = cycleStart + 1; i < dividerSlotIndex.length; i++)
          if (subtree_sizes[i].size < item.size)
            pos++

        // not changed
        if (pos == cycleStart) continue

        // skip duplicates
        while (item.size == subtree_sizes[pos].size) pos++

        // write
        var tmp = JSON.parse(JSON.stringify(subtree_sizes[pos]))
        subtree_sizes[pos].sender = item.sender
        subtree_sizes[pos].receiver = item.receiver
        subtree_sizes[pos].size = item.size
        item = tmp

        // repeat above to find a value to swap
        while (pos != cycleStart) {
          pos = cycleStart

          for (var i = cycleStart + 1; i < subtree_sizes.length; i++)
            if (subtree_sizes[i].size < item.size)
              pos++

          while (item.size == subtree_sizes[pos].size) pos++

          var tmp = JSON.parse(JSON.stringify(subtree_sizes[pos]))
          subtree_sizes[pos].sender = item.sender
          subtree_sizes[pos].receiver = item.receiver
          subtree_sizes[pos].size = item.size
          item = tmp
        }
      }

      this.sequence = []
      this.new_schedule = []
      for (var s = 0; s < subtree_sizes.length; s++) {
        this.sequence.push(this.find_cell(subtree_sizes[s].sender, "uplink"))
        this.new_schedule.push({ slot: { slot_offset: subtree_sizes[s].slot, channel_offset: subtree_sizes[s].ch }, row: subtree_sizes[s].row })
      }

      // mark the cells that no need to adjust
      for (var xx = 0; xx < this.sequence.length; xx++) {
        this.sequence[xx].adjusted = false
        if (this.sequence[xx].slot[0] == this.new_schedule[xx].slot.slot_offset) {
          this.sequence[xx].adjusted = true
        }
      }

      // cell in tmp area
      this.tmpCellIndex = -1
      this.rmTmpCellFlag = 0

      for (var k = 0; k < this.sequence.length; k++) {
        this.adjust_schedule(k, [k])
        if (this.tmpCellIndex != -1) {
          this.rmTmpCellFlag = 1
          this.adjust_schedule(this.tmpCellIndex, [])
        }
      }

    } else if (layer > 0) {
      subtree_sizes.sort((a, b) => (a.size < b.size) ? 1 : -1)
      for (var i = 0; i < subtree_sizes.length; i++) {
        if (subtree_sizes[i].row > 0 && subtree_sizes[i].size > 0) {
          var old_cell = this.find_cell(subtree_sizes[i].sender, "uplink")
          var ret = this.find_empty_subslot([old_cell.cell.sender, old_cell.cell.receiver], 1, { type: old_cell.cell.type, layer: old_cell.cell.layer })

          // A best, row 0 has space, just move
          if (ret.row == 0 && ret.is_optimal) {
            // console.log("move",subtree_sizes[i],"to",ret.slot)
            subtree_sizes[i].slot = ret.slot.slot_offset
            subtree_sizes[i].ch = ret.slot.channel_offset
            subtree_sizes[i].row = ret.row

            this.add_subslot(ret.slot, ret.subslot, { type: old_cell.cell.type, layer: old_cell.cell.layer, row: ret.row, sender: old_cell.cell.sender, receiver: old_cell.cell.receiver }, ret.is_optimal);
            this.remove_slot({ slot_offset: old_cell.slot[0], channel_offset: old_cell.slot[1] })
            this.total_edits++
            // if cannot simply move to row 0, check if can swap with a smallest cell or its smallest conflict cell
          } else {
            var swapCandidates = []
            var conflictCells = []
            var conflictSlots = []
            for (var j = 0; j < subtree_sizes.length; j++) {
              if (subtree_sizes[j].row == 0) {
                if (subtree_sizes[j].size < subtree_sizes[i].size) swapCandidates.push(subtree_sizes[j])
                if (subtree_sizes[j].receiver == subtree_sizes[i].receiver) {
                  conflictCells.push(subtree_sizes[j])
                  conflictSlots.push(subtree_sizes[j].slot)
                }
              }
            }

            swapCandidates.sort((a, b) => (a.size > b.size) ? 1 : -1)
            conflictCells.sort((a, b) => (a.size > b.size) ? 1 : -1)

            var minConflictCellSubtreeSize = 100
            if (conflictCells.length > 0) minConflictCellSubtreeSize = conflictCells[0].size

            var haveSwappedFlag = 0
            for (k = 0; k < swapCandidates.length; k++) {
              if (swapCandidates[k].size < minConflictCellSubtreeSize) {
                if (conflictSlots.indexOf(swapCandidates[k].slot) == -1) {
                  // console.log("move",subtree_sizes[i],"to", swapCandidates[k].slot,swapCandidates[k].ch)
                  var cell1 = this.find_cell(subtree_sizes[i].sender, "uplink")
                  var cell2 = this.find_cell(swapCandidates[k].sender, "uplink")

                  this.remove_slot({ slot_offset: cell1.slot[0], channel_offset: cell1.slot[1] })
                  this.remove_slot({ slot_offset: cell2.slot[0], channel_offset: cell2.slot[1] })
                  this.add_subslot({ slot_offset: cell2.slot[0], channel_offset: cell2.slot[1] }, { offset: 0, period: 1 }, { type: cell1.cell.type, layer: cell1.cell.layer, row: cell2.cell.row, sender: cell1.cell.sender, receiver: cell1.cell.receiver }, 1)

                  // find a cell to place the swapped cell
                  var ret = this.find_empty_subslot([cell2.cell.sender, cell2.cell.receiver], 1, { type: cell2.cell.type, layer: cell2.cell.layer })
                  this.add_subslot(ret.slot, ret.subslot, { type: cell2.cell.type, layer: cell2.cell.layer, row: ret.row, sender: cell2.cell.sender, receiver: cell2.cell.receiver }, ret.is_optimal);

                  subtree_sizes[i].slot = cell2.slot[0]
                  subtree_sizes[i].ch = cell2.slot[1]
                  subtree_sizes[i].row = cell2.cell.row

                  swapCandidates[k].slot = ret.slot.slot_offset
                  swapCandidates[k].ch = ret.slot.channel_offset
                  swapCandidates[k].row = ret.row

                  haveSwappedFlag = 1
                  if (ret.slot.slot_offset == cell1.slot[0] && ret.slot.channel_offset == cell1.slot[1]) this.total_edits += 3
                  else this.total_edits += 2
                  break
                }
              }
            }
            if (!haveSwappedFlag) {
              // no swap candidate works, swap with its smallest conflict cell
              if (minConflictCellSubtreeSize < subtree_sizes[i].size) {
                // console.log("move",subtree_sizes[i],"to", conflictCells[0].slot,conflictCells[0].ch)
                var cell1 = this.find_cell(subtree_sizes[i].sender, "uplink")
                var cell2 = this.find_cell(conflictCells[0].sender, "uplink")

                this.remove_slot({ slot_offset: cell1.slot[0], channel_offset: cell1.slot[1] })
                this.remove_slot({ slot_offset: cell2.slot[0], channel_offset: cell2.slot[1] })
                this.add_subslot({ slot_offset: cell2.slot[0], channel_offset: cell2.slot[1] }, { offset: 0, period: 1 }, { type: cell1.cell.type, layer: cell1.cell.layer, row: cell2.cell.row, sender: cell1.cell.sender, receiver: cell1.cell.receiver }, 1)

                // find a cell to place the swapped cell
                var ret = this.find_empty_subslot([cell2.cell.sender, cell2.cell.receiver], 1, { type: cell2.cell.type, layer: cell2.cell.layer })
                this.add_subslot(ret.slot, ret.subslot, { type: cell2.cell.type, layer: cell2.cell.layer, row: ret.row, sender: cell2.cell.sender, receiver: cell2.cell.receiver }, ret.is_optimal);

                subtree_sizes[i].slot = cell2.slot[0]
                subtree_sizes[i].ch = cell2.slot[1]
                subtree_sizes[i].row = cell2.cell.row

                conflictCells[0].slot = ret.slot.slot_offset
                conflictCells[0].ch = ret.slot.channel_offset
                conflictCells[0].row = ret.row

                if (ret.slot.slot_offset == cell1.slot[0] && ret.slot.channel_offset == cell1.slot[1]) this.total_edits += 3
                else this.total_edits += 2
              }
            }
          }
        }
      }
    }

    console.log("    adjust subtree distribution: " + this.total_edits + " edits")
    return this.total_edits
  }



  // adjust the order of cells of one partition by subtree size
  // determine the order by size and then do a `rejoin` process to compute the optimal schedule
  this.adjust_subtree_distribution_v0 = function (type, layer) {
    var schedule_backup = JSON.parse(JSON.stringify(this.schedule))
    var used_subslot_backup = JSON.parse(JSON.stringify(this.used_subslot))
    var subtree_sizes = this.get_subtree_size_list(type, layer)
    subtree_sizes.sort((a, b) => (a.size < b.size) ? 1 : -1)
    // var subtree_sizes = this.get_subtree_size_list_cycle_sort(type, layer)

    this.sequence = []
    for (var i = 0; i < subtree_sizes.length; i++) {
      // rejoin sequence
      var cell = this.find_cell(subtree_sizes[i].sender, "uplink")
      this.sequence.push(cell)
      // reset schedule
      this.schedule[subtree_sizes[i].slot][subtree_sizes[i].ch] = new Array(SUBSLOTS)
      this.remove_usedsubslot(cell.cell.sender, cell.cell.receiver, cell.cell.type)
    }

    // compute new optimal schedule
    this.new_schedule = []
    for (var j = 0; j < this.sequence.length; j++) {
      var cell = this.sequence[j]
      var ret = this.find_empty_subslot([cell.cell.sender, cell.cell.receiver], 1, { type: cell.cell.type, layer: cell.cell.layer })
      this.new_schedule.push(ret)
      this.add_subslot(ret.slot, ret.subslot, { row: ret.row, type: "uplink", layer: cell.cell.layer, sender: cell.cell.sender, receiver: cell.cell.receiver }, ret.is_optimal);
    }
    // restore old schedule
    this.schedule = JSON.parse(JSON.stringify(schedule_backup))
    this.used_subslot = JSON.parse(JSON.stringify(used_subslot_backup))

    // mark the cells that no need to adjust
    for (var xx = 0; xx < this.sequence.length; xx++) {
      this.sequence[xx].adjusted = false
      if (this.sequence[xx].slot[0] == this.new_schedule[xx].slot.slot_offset) {
        this.sequence[xx].adjusted = true
      }
    }

    this.total_edits = 0

    // cell in tmp area
    this.tmpCellIndex = -1
    this.rmTmpCellFlag = 0
    // still use the cycle sort idea, but try to insert into an idle cell first, then swap;
    // we only care the slot order, which channel doesn't matter.
    for (var k = 0; k < this.sequence.length; k++) {
      this.adjust_schedule(k, [k])
      if (this.tmpCellIndex != -1) {
        this.rmTmpCellFlag = 1
        this.adjust_schedule(this.tmpCellIndex, [])
      }
    }
    console.log("adjust subtree distribution of " + type + " layer " + layer + ": " + this.total_edits + " edits")
    return this.total_edits
  }

  // adjust old schedule to new_schedule, with the idea of cycle sort to minimize schedule edits number
  // assume not support "specify a future time" in schedule edit
  this.adjust_schedule = function (k, history) {
    var old_cell = this.sequence[k]
    var new_slot = this.new_schedule[k]
    if (old_cell.adjusted) return

    // check if that slot has an idle channel to use or if there exists a conflict cell
    var idleChannels = []
    var conflictCellIndex = -1
    var swapCandidates = []
    for (var c = 0; c < this.channelRows[new_slot.row].length; c++) {
      var ch = this.channelRows[new_slot.row][c]
      // idle: not used
      if (this.schedule[new_slot.slot.slot_offset][ch][0] == null) {
        idleChannels.push(ch)
      } else {
        var index = 0
        for (var ii = 0; ii < this.sequence.length; ii++)
          if (this.sequence[ii].cell.sender == this.schedule[new_slot.slot.slot_offset][ch][0].sender)
            index = ii

        // conflict (same parent/receiver) or swap candidate (havent adjusted)
        if (this.schedule[new_slot.slot.slot_offset][ch][0].receiver == old_cell.cell.receiver) {
          conflictCellIndex = index
        }
        else if (!this.sequence[index].adjusted)
          swapCandidates.push(index)
      }
    }
    var tmpFlag = 0

    var dstSlot = new_slot.slot.slot_offset
    // move the conflict cell away and insert
    if (conflictCellIndex != -1) {
      // loop detected, move to tmp area
      if (history.indexOf(conflictCellIndex) != -1) {
        this.add_subslot({ slot_offset: 10, channel_offset: 10 }, { offset: 0, period: 1 }, this.sequence[k].cell, 0)
        this.tmpCellIndex = k
        dstSlot = 10
        tmpFlag = 1
      } else {
        history.push(conflictCellIndex)
        this.adjust_schedule(conflictCellIndex, history)
        this.add_subslot({ slot_offset: new_slot.slot.slot_offset, channel_offset: this.sequence[conflictCellIndex].slot[1] }, { offset: 0, period: 1 }, { row: new_slot.row, type: old_cell.cell.type, layer: old_cell.cell.layer, sender: old_cell.cell.sender, receiver: old_cell.cell.receiver }, 1)
      }

      // insert into a idle cell
    } else if (idleChannels.length > 0) {
      this.add_subslot({ slot_offset: new_slot.slot.slot_offset, channel_offset: idleChannels[0] }, { offset: 0, period: 1 }, { row: new_slot.row, type: old_cell.cell.type, layer: old_cell.cell.layer, sender: old_cell.cell.sender, receiver: old_cell.cell.receiver }, 1)

      // move a swap candidate away and insert
    } else if (swapCandidates.length > 0) {
      history.push(swapCandidates[0])
      this.adjust_schedule(swapCandidates[0], history)
      this.add_subslot({ slot_offset: new_slot.slot.slot_offset, channel_offset: this.sequence[swapCandidates[0]].slot[1] }, { offset: 0, period: 1 }, { row: new_slot.row, type: old_cell.cell.type, layer: old_cell.cell.layer, sender: old_cell.cell.sender, receiver: old_cell.cell.receiver }, 1)
    }

    if (this.tmpCellIndex != -1 && this.rmTmpCellFlag) {
      this.remove_slot({ slot_offset: 10, channel_offset: 10 })
      this.tmpCellIndex = -1
      this.rmTmpCellFlag = 0
    } else {
      this.remove_slot({ slot_offset: old_cell.slot[0], channel_offset: old_cell.slot[1] })
    }
    this.total_edits++
    if (!tmpFlag) old_cell.adjusted = true

    // console.log(old_cell.cell.sender, old_cell.slot[0],'->',dstSlot)
  }

  // adjust partitions to the left to leave space
  this.adjust_gap = function (node) {
    var cell = this.find_cell(node, "uplink")
    // idle slots number
    var have_idle_slots = []
    var total_idle_slots = 0
    for (var i = Object.keys(this.partition[0].uplink).length - 1; i > cell.cell.layer; i--) {
      var idle_slots = this.get_idle_slots(2, "uplink", i)
      if (idle_slots > 0) {
        have_idle_slots.push(i)
        total_idle_slots += idle_slots
      }
    }
    // if(total_idle_slots)

  }

  // see minimized_used_slots section of the paper
  this.minimize_used_slots = function (type, layer) {
    var edits = 0
    if (layer == 0) return edits
    // nodes in last layer
    var cells = []
    var groups = {}
    var n = 0
    for (var i = 0; i < this.used_subslot.length; i++) {
      if (this.used_subslot[i].cell.type == type && this.used_subslot[i].cell.layer == layer) {
        n++
        var cell = this.used_subslot[i]
        var parent = (type == "uplink") ? cell.cell.receiver : cell.cell.sender
        if (groups[parent] == null) groups[parent] = [cell]
        else groups[parent].push(cell)
      }
    }
    if (n == 0) return edits

    for (var i in groups) cells.push(groups[i])
    cells.sort((a, b) => (a.length < b.length) ? 1 : -1)

    var alpha = cells[0].length
    var beta = Math.ceil(n / 6)
    var rho = Math.max(alpha, beta)
    var cur_slots = this.count_used_slots(type, layer)
    console.log("minimum needed slots is " + rho + ", using " + cur_slots + " now")
    if (rho < cur_slots) {
      console.log("perform used slots minimization");
      return edits
    }

    var min_slot_list = []
    if (rho < (this.partition[0][type][layer].end - this.partition[0][type][layer].start)) {
      for (var r = 0; r < rho; r++)
        min_slot_list.push(this.partition[0][type][layer].end - 1 - r)
    } else {
      // if(rho<)
    }


    // // schedule larger branches first
    // children.sort((a, b) => (a.length < b.length) ? 1 : -1)
    // // the minimum used slots list
    // var new_slot_list = []
    // var alpha = children[0].length
    // // assume row 0 is enough for now, 8 channels
    // var beta = Math.ceil(n/8)
    // var rho = Math.max(alpha, beta)
    // for(var r=0;r<rho;r++) new_slot_list.push(this.partition[0][type][layer].end-1-r)

    // for(var k=0;k<children.length;k++) {
    //   for(var l=0;l<children[k].length;l++) {
    //     var cell = this.find_cell(children[k][l], type).cell
    //     // reset this.used_subslot[i]
    //     this.remove_usedsubslot(cell.sender, cell.receiver, cell.type)
    //     var ret = this.find_empty_subslot([cell.sender, cell.receiver], 1, {type: cell.type, layer: cell.layer})        
    //     sch.add_subslot(ret.slot, ret.subslot, {row:ret.row,type:type,layer:layer,sender:cell.sender,receiver:cell.receiver}, ret.is_optimal)
    //     new_slot_list.push(ret.slot.slot_offset)
    //   }
    // }
    // new_slot_list = Array.from(new Set(new_slot_list)).sort()
    // var new_used_slots_cnt = this.count_used_slots(type, layer)

    // // restore old schedule
    // this.schedule = JSON.parse(JSON.stringify(schedule_backup))
    // this.used_subslot = JSON.parse(JSON.stringify(used_subslot_backup))

    // // not using minimum slots, need adjustment,
    // // adjust cells not in the new_slot_list, still in the order of branch size
    // var edits = 0

    // if(new_used_slots_cnt < old_used_slots_cnt) {
    //   for(var k=0;k<children.length;k++) {
    //     for(var l=0;l<children[k].length;l++) {
    //       var old_cell = this.find_cell(children[k][l], type)

    //       if(new_slot_list.indexOf(old_cell.slot[0]) == -1) {
    //         var ret = this.find_empty_subslot([old_cell.cell.sender, old_cell.cell.receiver], 1, {type:old_cell.cell.type, layer:old_cell.cell.layer})

    //         // able to move to the right place
    //         if(new_slot_list.indexOf(ret.slot.slot_offset) != -1) {
    //           this.add_subslot(ret.slot, ret.subslot, {type:old_cell.cell.type,layer:old_cell.cell.layer,row:ret.row,sender:old_cell.cell.sender,receiver:old_cell.cell.receiver}, ret.is_optimal);
    //           this.remove_slot({slot_offset:old_cell.slot[0], channel_offset:old_cell.slot[1]})
    //           edits++
    //         // need to move a cell away
    //         } else {
    //           var brothers = JSON.parse(JSON.stringify(this.topo[old_cell.cell.receiver]))
    //           brothers.splice(brothers.indexOf(old_cell.cell.sender), 1)
    //           var brothers_used_slots = {}
    //           // no brothers occupied slots
    //           var available_slots = []
    //           for(var b=0;b<brothers.length;b++)
    //             brothers_used_slots[this.find_cell(brothers[b], type).slot[0]] = 1

    //           for(var s=0;s<new_slot_list.length;s++)
    //             if(brothers_used_slots[new_slot_list[s]] == null)
    //               available_slots.push(new_slot_list[s])
    //           available_slots.sort((a, b) => b - a)

    //           var done = 0
    //           // find the cell to be moved
    //           for(var ss=0;ss<available_slots.length;ss++) {
    //             var slot = available_slots[ss]
    //             var channels = []
    //             for(var r=0;r<ROWS;r++)
    //               if(slot>=this.partition[r][type][layer].start && slot<this.partition[r][type][layer].end)
    //                 channels.push.apply(channels, this.channelRows[r])
    //             for(var c=0;c<channels.length;c++) {
    //               var ch = channels[c]
    //               var cell = this.find_cell(this.schedule[slot][ch][0].sender, type)
    //               var ret = this.find_empty_subslot([cell.cell.sender, cell.cell.receiver], 1, {type:cell.cell.type, layer:cell.cell.layer})
    //               if(new_slot_list.indexOf(ret.slot.slot_offset) != -1) {
    //                 // move away
    //                 this.add_subslot(ret.slot, ret.subslot, {type:cell.cell.type,layer:cell.cell.layer,row:ret.row,sender:cell.cell.sender,receiver:cell.cell.receiver}, ret.is_optimal);
    //                 this.remove_slot({slot_offset:cell.slot[0], channel_offset:cell.slot[1]})
    //                 // move old_cell here
    //                 this.add_subslot({slot_offset:cell.slot[0], channel_offset:cell.slot[1]}, {offset:0, period:1}, {type:old_cell.cell.type,layer:old_cell.cell.layer,row:cell.cell.row,sender:old_cell.cell.sender,receiver:old_cell.cell.receiver}, ret.is_optimal)
    //                 this.remove_slot({slot_offset:old_cell.slot[0], channel_offset:old_cell.slot[1]})
    //                 done = 1
    //                 edits += 2
    //                 break
    //               }
    //             }
    //             if(done) break
    //           }
    //         }  
    //       }
    //     }
    //   }
    //   console.log("    minimize used slots: "+edits+" edits")
    // } else {
    //   console.log("    already using minimum slots")
    // }
    // return edits
  }

  //generate a shuffled slot list to iterate
  this.shuffle_slots = function (start, end) {
    if(!start && !end) {
      start = RESERVED
      end = this.slotFrameLength
    }

    var shuffled_slots = [];
    for (var slot = start; slot < end; ++slot) {
      for (var c in this.channels) {
        var ch = this.channels[c];
        shuffled_slots.push({ slot_offset: slot, channel_offset: ch });
      }
    }
    for (var i = shuffled_slots.length - 1; i > 0; i--) {
      var j = Math.floor(Math.random() * (i + 1));
      var temp = shuffled_slots[i];
      shuffled_slots[i] = shuffled_slots[j];
      shuffled_slots[j] = temp;
    }
    var schedule = this.schedule;
    //pass into sort(func)
    //sort the shuffled list by subslot occupation
    shuffled_slots.sort(function (a, b) {
      var ca = 0, cb = 0;
      var a_sublist = schedule[a.slot_offset][a.channel_offset];
      var b_sublist = schedule[b.slot_offset][b.channel_offset];
      for (var sub = 0; sub < SUBSLOTS; ++sub) {
        if (a_sublist[sub] == null) {
          ++ca;
        }
        if (b_sublist[sub] == null) {
          ++cb;
        }
      }
      return ca - cb;
    });
    return shuffled_slots;
  }

  this.count_used_slotss = function () {
    var cnt = 0;
    for (var slot = 0; slot < 127; slot++) {
      var flag = 0;
      for (var ch = 1; ch < 17; ch++) {
        if (this.schedule[slot][ch][0] != null) {
          flag = 1
          break
        }
      }
      if (flag == 1)
        cnt++
    }
    return cnt
  }

  this.count_multi_ch_slots = function () {
    var cnt = 0;
    for (var slot = 0; slot < 127; slot++) {
      var tmp = 0;
      for (var ch = 1; ch < 8; ch++) {
        if (this.schedule[slot][ch][0] != null) {
          tmp++
        }
      }
      if (tmp > 1)
        cnt++
    }
    return cnt
  }


  this.LDSF = function (nodes_list, info) {
    var parent = (info.type == "uplink") ? nodes_list[1] : parent = nodes_list[0]
    var slot_list = []
    var block_size = 5
    var block_num = 120 / block_size
    var block_id
    var block_list = []

    // 1 cell per link, from root to leaf
    if (parent == 1) {
      // block_id = Math.floor(Math.random() * block_num)
      block_id = block_numz
      if (block_id % 2 != 0) block_id -= 1
      if (info.type == "uplink") {
        for (var b = block_id; b >= 0; b -= 2) block_list.push(b)
        for (var b = block_num; b > block_id; b -= 2) block_list.push(b)
      } else {
        for (var b = block_id; b <= block_num; b += 2) block_list.push(b)
        for (var b = 0; b < block_id; b += 2) block_list.push(b)
      }
    } else {
      var parent_cell = this.find_cell(parent, info.type)
      if (parent_cell != null) {
        var parent_block_id = Math.floor(parent_cell.slot[0] / block_size)
        // console.log("parent:",parent, "slot:",parent_cell.slot[0], "blkid:",parent_block_id)

        
        if (info.type == "uplink") {
          if (parent_block_id == 0) block_id = block_num
          else block_id = parent_block_id - 1

          for (var b = block_id; b >= 0; b -= 2) block_list.push(b)
          for (var b = block_num; b > block_id; b -= 2) block_list.push(b)
        } else {
          if (parent_block_id == block_num) block_id = 0
          else block_id = parent_block_id + 1

          for (var b = block_id; b <= block_num; b += 2) block_list.push(b)
          for (var b = 0; b < block_id; b += 2) block_list.push(b)
        }

      }
    }

    // // flow level scheduling, from leaf to root
    // block_id = Math.floor(Math.random()*block_num)
    // // leaf
    // if(this.topology[node]==null) {
    //   // even
    //   if(info.layer%2==0) {
    //     if(block_id%2!=0) block_id-=1
    //     for(var b=block_id;b>=0;b-=2) block_list.push(b)
    //     for(var b=block_num;b>block_id;b-=2) block_list.push(b)
    //   } else {
    //     // odd
    //     if(block_id%2!=0) {
    //       if(block_id==0) block_id++
    //       else block_id--
    //     }
    //     for(var b=block_id;b>=0;b-=2) block_list.push(b)
    //     for(var b=block_num;b>block_id;b-=2) block_list.push(b)
    //   }

    // } else {
    //   this.find_cell()
    // }


    for (var i = 0; i < block_list.length; i++) {
      for (var s = block_list[i] * block_size; s < (block_list[i] + 1) * block_size; s++) {
        for (var ch = 1; ch < 17; ch++) {
          slot_list.push({ slot_offset: s, channel_offset: ch })
        }
      }
    }

    return slot_list
  }

  // LLSF scheduler, return the slot list in the left/right of its parent's uplink/downlink slot
  this.LLSF = function (parent, type) {
    var slot_list = []
    var slots = []
    // gateway is 0 in simulation, 1 in testbed
    if (parent == 0) {
      return this.shuffle_slots()
    }
    var parent_cell = this.find_cell(parent, type)
    if (type == "uplink") {
      for (var i = parent_cell.slot[0] - 1; i >= 10; i--) slots.push(i)
      for (var j = 127 - 1; j > parent_cell.slot[0]; j--) slots.push(j)
    } else {
      for (var i = parent_cell.slot[0] + 1; i < 127; i++) slots.push(i)
      for (var j = 10; j < parent_cell.slot[0]; j++) slots.push(j)
    }
    for (var s = 0; s < slots.length; s++) {
      var slot = slots[s]
      for (var ch = 1; ch < 17; ch++) {
        slot_list.push({ slot_offset: slot, channel_offset: ch })
      }
    }
    return slot_list
  }

  this.count_used_slots = function (type, layer) {
    var slots = {}
    for (var i = 0; i < this.used_subslot.length; i++) {
      if (this.used_subslot[i].is_optimal && this.used_subslot[i].cell.type == type && this.used_subslot[i].cell.layer == layer) {
        var cell = this.used_subslot[i]
        if (slots[cell.slot[0]] == null) slots[cell.slot[0]] = [cell]
        else slots[cell.slot[0]].push(cell)
      }
    }
    return Object.keys(slots).length
  }

  // put unaligned links back
  this.adjust_unaligned_cells = function () {
    var used_subslot = JSON.parse(JSON.stringify(this.used_subslot));
    for (var j = 0; j < used_subslot.length; j++) {
      if (!used_subslot[j].is_optimal) {
        var old = used_subslot[j]
        // console.log("adjusting",old)
        var ret = this.find_empty_subslot([old.cell.sender, old.cell.receiver], 1, { type: old.cell.type, layer: old.cell.layer }, old.cell.row)
        if (ret.is_optimal) {
          // console.log("to new position",ret)
          this.add_subslot(ret.slot, ret.subslot, { row: old.cell.row, type: old.cell.type, layer: old.cell.layer, sender: old.cell.sender, receiver: old.cell.receiver }, ret.is_optimal);
          this.remove_subslot({ slot_offset: old.slot[0], channel_offset: old.slot[1] }, { offset: old.subslot[0], period: old.subslot[1] })
        }
      }
    }
  }

  this.dynamic_partition_adjustment = function () {
    // highest layer of non-optmial cells
    var highest_layer = Object.keys(this.partition[0]['uplink']).length - 1
    for (var row = 0; row < 1; row++) {
      this.adjust(row, 'uplink', highest_layer)
      // this.adjust(row,'downlink',highest_layer)
    }
    // this.reset_partition_changes()

    console.log('Partitions offset adjustment summary:', this.adjustment_summary)

    // make a real/deep copy! add or remove will change sch.used_subslot length
    var used_subslot = JSON.parse(JSON.stringify(this.used_subslot));
    var cnt = 0
    // put unaligned links back
    for (var j = 0; j < used_subslot.length; j++) {
      if (!used_subslot[j].is_optimal) {
        var old = used_subslot[j]
        this.remove_subslot({ slot_offset: old.slot[0], channel_offset: old.slot[1] }, { offset: old.subslot[0], period: old.subslot[1] })

        // console.log("adjusting",old)
        var ret = this.find_empty_subslot([old.cell.sender, old.cell.receiver], 1, { type: old.cell.type, layer: old.cell.layer }, old.cell.row)
        // console.log("to new position",ret)
        this.add_subslot(ret.slot, ret.subslot, { row: old.cell.row, type: old.cell.type, layer: old.cell.layer, sender: old.cell.sender, receiver: old.cell.receiver }, ret.is_optimal);

        cnt++
      }
    }
    // console.log("Put", cnt, "non-optimal cells back")
  }

  //3-d searcher
  this.find_empty_subslot = function (nodes_list, period, info) {
    var slots_list;
    var checkOrder = 1
    var parent = (info.type == "uplink") ? nodes_list[1] : parent = nodes_list[0]
    rows = []
    for(var r=0;r<ROWS;r++) rows.push(r)

    // link options
    if(!info.option) {
      if (info.type == "beacon") slots_list = this.inpartition_slots(0, info, 0)
      else slots_list = this.shuffle_slots(this.partition.control.start, this.partition.control.end)
      
      for (var i = 0; i < slots_list.length; ++i) {
        var slot = slots_list[i];
        for (var offset = 0; offset < period; ++offset) {
          if (this.available_subslot(nodes_list, slot, { period: period, offset: offset }, info, 0)) {
            return { slot: slot, subslot: { offset: offset, period: period }, row: 0, is_optimal: 1 }
          }
        }
      }
      return
    }

    // random
    if (this.algorithm == "random") {
      if (info.type == "beacon") slots_list = this.inpartition_slots(0, info, 0)
      else slots_list = this.shuffle_slots()
      for (var i = 0; i < slots_list.length; ++i) {
        var slot = slots_list[i];
        for (var offset = 0; offset < period; ++offset) {
          if (this.available_subslot(nodes_list, slot, { period: period, offset: offset }, info, 0)) {
            return { slot: slot, subslot: { offset: offset, period: period }, row: 0, is_optimal: 1 }
          }
        }
      }
      return
    }
    // LLSF, minimize slot gaps
    if (this.algorithm == "LLSF") {
      if (info.type == "beacon") slots_list = this.inpartition_slots(0, info, 0)
      else slots_list = this.LLSF(parent, info.type)
      for (var i = 0; i < slots_list.length; ++i) {
        var slot = slots_list[i];
        for (var offset = 0; offset < period; ++offset) {
          if (this.available_subslot(nodes_list, slot, { period: period, offset: offset }, info, 0)) {
            return { slot: slot, subslot: { offset: offset, period: period }, row: 0, is_optimal: 1 }
          }
        }
      }
      return
    }

    if (this.algorithm == "LDSF") {
      slots_list = this.LDSF(nodes_list, info)
      for (var i = 0; i < slots_list.length; ++i) {
        var slot = slots_list[i];
        for (var offset = 0; offset < period; ++offset) {
          if (this.available_subslot(nodes_list, slot, { period: period, offset: offset }, info, 0)) {
            return { slot: slot, subslot: { offset: offset, period: period }, row: 0, is_optimal: 1 }
          }
        }
      }
      console.log(nodes_list, info, "not found")
      return
    }

    // partition
    for (var ii = 0; ii < ROWS; ii++) {
      r = rows[ii]
      slots_list = this.inpartition_slots(0, info, r);
      for (var i = 0; i < slots_list.length; ++i) {
        var slot = slots_list[i];
        for (var offset = 0; offset < period; ++offset) {
          if (this.available_subslot(nodes_list, slot, { period: period, offset: offset }, info, checkOrder)) {
            return { slot: slot, subslot: { offset: offset, period: period }, row: r, is_optimal: 1 }
          }
        }
      }
    }
    // console.log("No empty aligned slot")
    this.nonOptimalCount++;

    // assign in reserved area, row 0
    slots_list = this.inpartition_slots(1, { type: info.type, layer: 0 }, 0);
    for (var i = 0; i < slots_list.length; ++i) {
      var slot = slots_list[i];
      for (var offset = 0; offset < period; ++offset) {
        if (this.available_subslot(nodes_list, slot, { period: period, offset: offset }, info, 0)) {
          var ret = { slot: slot, subslot: { offset: offset, period: period }, row: 0, is_optimal: 0 }
          // console.log("find an alternative slot:",ret);
          return (ret);
        }
      }
    }

    console.log(nodes_list, info, "No empty slot found");
    slots_list=this.shuffle_slots()
    for(var i=0;i<slots_list.length;++i){
      var slot=slots_list[i];
      for(var offset=0;offset<period;++offset){
        if(this.available_subslot(nodes_list,slot,{period:period,offset:offset}, info, 0)){
          return {slot:slot,subslot:{offset:offset,period:period}, row:0, is_optimal:0}
        }
      }
    }
    this.isFull = true;
    return { slot: { slot_offset: 5, channel_offset: 10 }, row: 0, subslot: { offset: 0, period: 16 }, is_optimal: 0 };
  }

  this.remove_node = function (node) {
    for (var slot = 0; slot < this.slotFrameLength; ++slot) {
      for (var c in this.channels) {
        var ch = this.channels[c];
        for (var sub = 0; sub < SUBSLOTS; ++sub) {
          if (this.schedule[slot][ch][sub] != null) {
            if (
              this.schedule[slot][ch][sub].sender == node ||
              this.schedule[slot][ch][sub].receiver == node
            ) {
              // console.log("schedule["+slot+"]["+ch+"]["+sub+"] cleaned");
              this.schedule[slot][ch][sub] = null;
            }
          }
        }
      }
    }
    for (var i = 0; i < this.used_subslot.length; i++) {
      if (this.used_subslot[i].cell.sender == node || this.used_subslot[i].cell.receiver == node) {
        this.used_subslot.splice(i, 1)
        // array length will change
        i--
      }
    }
  }

  this.periodOffsetToSubslot = function (periodOffset, period) {
    return periodOffset * ((SUBSLOTS / period) % SUBSLOTS);
  }
}

module.exports = {
  create_scheduler: create_scheduler,
  SUBSLOTS: SUBSLOTS
};

