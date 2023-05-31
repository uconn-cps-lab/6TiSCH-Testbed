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

const RESERVED = 4; //Number of reserved, 0-gw beacon, 1-5 shared slots
const SUBSLOTS = 1;
const ALGORITHM = "V"

var schedule_preset = require("./schedule_preset.json")

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

// var old_id = find_old_id("00-12-4b-00-0c-61-f1-83")


const BEACON_SLOTS = 24
const MGMT_SLOTS = 60


function partition_init(sf) {
  //Beacon reserved version
  // var u_d = [Math.floor(sf - BEACON_SLOTS - RESERVED - MGMT_SLOTS) / 2,
  // Math.floor(sf - BEACON_SLOTS - RESERVED - MGMT_SLOTS) / 2];

  // beacon in mgmt version
  var u_d = [Math.floor(sf - RESERVED - MGMT_SLOTS) / 2,
  Math.floor(sf - RESERVED - MGMT_SLOTS) / 2];

  // var partition = {
  //   shared: { start: 0, end: RESERVED },
  //   beacon: { start: RESERVED, end: RESERVED + BEACON_SLOTS },
  //   uplink: { start: RESERVED + BEACON_SLOTS, end: RESERVED + BEACON_SLOTS + u_d[0] },
  //   management: { start: sf - u_d[1] - MGMT_SLOTS, end: sf - u_d[1] },
  //   downlink: { start: sf - u_d[1], end: sf },
  // }

  // beacon in mgmt version
  var partition = {
    shared: { start: 0, end: RESERVED },
    uplink: { start: RESERVED, end: RESERVED + u_d[0] },
    management: { start: sf - u_d[1] - MGMT_SLOTS, end: sf - u_d[1] },
    downlink: { start: sf - u_d[1], end: sf },
  }

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
  Cell = {type, sender, receiver}
  
  used_subslot = {slot: [slot_offset, ch_offset], subslot: [periord, offset], cell: cell}
  */
  if (!(this instanceof create_scheduler)) {
    return new create_scheduler(sf, ch);
  }

  console.log("create_scheduler(" + sf + "," + ch + ")", ALGORITHM);
  this.slotFrameLength = sf;
  this.beacon_slots = [61, 62, 63, 64, 65, 67, 68, 70, 71, 73, 74, 75, 76, 77, 79, 80, 82, 83, 84, 85, 87, 89, 90, 91, 92]
  this.channels = ch;
  this.algorithm = ALGORITHM
  this.schedule_preset = schedule_preset
  this.schedule_preset_table = new Array(sf)
  this.schedule = new Array(sf);
  // { parent: [children] }, mainly for count subtree size
  // mainly for send cells to cloud
  this.used_subslot = []

  for (var i = 0; i < sf; ++i) {
    this.schedule[i] = new Array(16);
    this.schedule_preset_table[i] = new Array(16);
  }
  for (var c in this.channels) {
    var ch = this.channels[c];
    for (var slot = 0; slot < this.slotFrameLength; ++slot) {
      this.schedule[slot][ch] = new Array(SUBSLOTS)
      this.schedule_preset_table[slot][ch] = new Array(SUBSLOTS)
    }
  }

  this.partition = partition_init(sf);

  // fill preset schedule table
  for (let cell of this.schedule_preset) {
    // console.log(cell)
    this.schedule_preset_table[cell.slot.slot_offset][cell.slot.channel_offset][cell.subslot.offset] = {
      sender: cell.sender,
      receiver: cell.receiver,
    }
  }


  this.add_slot = function (slot, cell) {
    this.add_subslot(slot, { offset: 0, period: 1 }, cell);
  }

  this.add_subslot = function (slot, subslot, cell) {
    var sub_start = subslot.offset * SUBSLOTS / subslot.period;
    var sub_end = (subslot.offset + 1) * SUBSLOTS / subslot.period;

    for (var sub = sub_start; sub < sub_end; ++sub) {
      this.schedule[slot.slot_offset][slot.channel_offset][sub] = {
        type: cell.type,
        sender: cell.sender,
        receiver: cell.receiver,
        option: cell.option
      }
      this.schedule_preset_table[slot.slot_offset][slot.channel_offset][sub] = {
        type: cell.type,
        sender: cell.sender,
        receiver: cell.receiver,
        option: cell.option
      }
    }

    this.used_subslot.push({ slot: [slot.slot_offset, slot.channel_offset], subslot: [subslot.offset, subslot.period], cell: cell })
  }

  this.remove_slot = function (slot) {
    this.remove_subslot(slot, { offset: 0, period: 1 });
  }

  this.remove_subslot = function (slot, subslot) {
    var sub_start = subslot.offset * SUBSLOTS / subslot.period;
    var sub_end = (subslot.offset + 1) * SUBSLOTS / subslot.period;
    for (var sub = sub_start; sub < sub_end; ++sub) {
      this.schedule[slot.slot_offset][slot.channel_offset][sub] = null;
      this.schedule_preset_table[slot.slot_offset][slot.channel_offset][sub] = null;
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
  this.available_subslot = function (nodes_list, slot, subslot, info) {
    if (slot.slot_offset < RESERVED) return false;
    // this.partition.beacon.start = 125
    // this.partition.beacon.end = 127
    //if is beacon, we want it always the first channel in the list;
    //it actually doesn't matter since hardware-wise it's hardcoded to beacon channel
    //but to make it consistant in scheduler table...
    if (info.type == "beacon" && slot.channel_offset != this.channels[0]) return false;

    //Beacon reserved version: beacon partition can only be utilized by beacon

    if (info.type == "beacon") {
      if (this.beacon_slots.indexOf(slot.slot_offset) == -1) return false
    }


    //check if this slot-channel is assigned
    var sub_start = subslot.offset * SUBSLOTS / subslot.period;
    var sub_end = (subslot.offset + 1) * SUBSLOTS / subslot.period;
    for (var sub = sub_start; sub < sub_end; ++sub) {
      if (this.schedule_preset_table[slot.slot_offset][slot.channel_offset][sub] != null)
        return false;
    }

    //check if this slot (any channel) is assigned to beacon
    //or is assigned to the members
    for (var c in this.channels) {
      var ch = this.channels[c];
      for (var sub = sub_start; sub < sub_end; ++sub) {
        if (this.schedule_preset_table[slot.slot_offset][ch][sub] == null)
          continue;


        if (nodes_list.indexOf(this.schedule_preset_table[slot.slot_offset][ch][sub].sender) != -1) return false;
        if (nodes_list.indexOf(this.schedule_preset_table[slot.slot_offset][ch][sub].receiver) != -1) return false;
      }
    }
    return true;
  }

  //3-d searcher
  this.find_empty_subslot = function (nodes_list_eui, period, info) {
    // console.log(nodes_list_eui)
    nodes_list = [old_id(nodes_list_eui[0]), old_id(nodes_list_eui[1])]
    // console.log("new", nodes_list)
    var slots_list = [];

    if (info.type == "beacon") {
      for (s of this.beacon_slots) {
        slots_list.push({ slot_offset: s, channel_offset: 1 })
      }
    }
    // console.log(this.brea)
    else slots_list = this.shuffle_slots(6, 60)

    // console.log(slots_list)
    for (var i = 0; i < slots_list.length; ++i) {
      var slot = slots_list[i];
      for (var offset = 0; offset < period; ++offset) {
        if (this.available_subslot(nodes_list, slot, { period: period, offset: offset }, info)) {
          return { slot: slot, subslot: { offset: offset, period: period } }
        }
      }
    }
    console.log("cannot find empty subslot", nodes_list, info)
    return { slot: slot, subslot: { offset: offset, period: period } }
  }

  this.find_preset_cells = function (node_eui) {
    var ret = []
    var node = old_id(node_eui)
    // console.log(node)
    for (cell of this.schedule_preset) {
      // console.log(cell)
      if (cell.sender == node) {
        ret.push({
          slot: cell.slot,
          subslot: cell.subslot,
          opt: 0x21
        })
      }
      if (cell.receiver == node) {
        ret.push({
          slot: cell.slot,
          subslot: cell.subslot,
          opt: 0x0A
        })
      }
    }
    return ret
  }

  //generate a shuffled slot list to iterate
  this.shuffle_slots = function (start, end) {
    if (!start && !end) {
      start = RESERVED
      end = this.slotFrameLength
    }

    var shuffled_slots = [];
    for (var slot = start; slot < end; ++slot) {
      for (var c in this.channels) {
        var ch = this.channels[c];
        if (ch == 1) continue // skip beacon channel
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

          if (this.schedule_preset_table[slot][ch][sub] != null) {
            if (
              this.schedule_preset_table[slot][ch][sub].sender == node ||
              this.schedule_preset_table[slot][ch][sub].receiver == node
            ) {
              // console.log("schedule["+slot+"]["+ch+"]["+sub+"] cleaned");
              this.schedule_preset_table[slot][ch][sub] = null;
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

// var sch = create_scheduler(100, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])
// console.log(sch.beacon_slots.length)
// console.log(sch.find_preset_cells("00-12-4b-00-11-a7-21-06"))
// var cell = sch.find_empty_subslot([3], 1, {type: "beacon", layer: 0})
// console.log(cell)
// cell = sch.find_empty_subslot([3,1], 1, {type: "uplink", layer: 0})
// console.log(cell)
// cell =  sch.find_empty_subslot([1,3], 1, {type: "downlink", layer: 0})
// console.log(cell)

module.exports = {
  create_scheduler: create_scheduler,
  SUBSLOTS: SUBSLOTS
};

