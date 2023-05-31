var coap = require("coap")
cloud = require("./cloud.json");
// var network_manager = require('./network_manager')

const MAC_LINK_OPTION_TX = 0x01
const MAC_LINK_OPTION_RX = 0x02

function tlv_to_schedule_params(tlvdata) {

      var sched_entry = { seq: 0, totalLinkCount: 0, retrievedLinkCount: 0, startLinkIdx: 0, links: [] };

      if (tlvdata.length >= 4) {
            var idx = 0;
            var linkCount = 0;

            sched_entry.seq = tlvdata[idx];
            idx += 1;
            sched_entry.totalLinkCount = tlvdata[idx];
            idx += 1;
            sched_entry.retrievedLinkCount = tlvdata[idx];
            idx += 1;
            sched_entry.startLinkIdx = tlvdata[idx];
            idx += 1;

            while (idx < tlvdata.length) {

                  sched_entry.links[linkCount] = {};
                  var link = sched_entry.links[linkCount];

                  link.slotframe_handle = tlvdata[idx];
                  idx++;

                  if ((tlvdata.length - idx) < 1) break;
                  link.link_option = tlvdata[idx];
                  idx++;

                  if ((tlvdata.length - idx) < 1) break;
                  link.link_type = tlvdata[idx];
                  idx++;

                  if ((tlvdata.length - idx) < 1) break;
                  link.period = tlvdata[idx];
                  idx++;

                  if ((tlvdata.length - idx) < 1) break;
                  link.periodOffset = tlvdata[idx];
                  idx++;

                  if ((tlvdata.length - idx) < 2) break;
                  link.link_id = tlvdata[idx] + tlvdata[idx + 1] * 256;  //LE format
                  idx += 2;

                  if ((tlvdata.length - idx) < 2) break;
                  link.timeslot = tlvdata[idx] + tlvdata[idx + 1] * 256;  //LE format
                  idx += 2;

                  if ((tlvdata.length - idx) < 2) break;
                  link.channel_offset = tlvdata[idx] + tlvdata[idx + 1] * 256;  //LE format
                  idx += 2;

                  if ((tlvdata.length - idx) < 2) break;
                  link.peer_addr = tlvdata[idx] + tlvdata[idx + 1] * 256;  //LE format
                  idx += 2;

                  linkCount++;
            }

            if (linkCount != sched_entry.retrievedLinkCount || linkCount > sched_entry.totalLinkCount) {
                  sched_entry.totalLinkCount = 0;
                  sched_entry.retrievedLinkCount = 0;
            }
      }

      return sched_entry;
}

function construct_payload(slots) {
      var buf = Buffer.alloc(slots.length * 7)
      for (var i = 0; i < slots.length; ++i) {
            buf.writeUInt16BE(slots[i].timeslot, i * 7 + 0);
            buf.writeUInt16BE(slots[i].channel, i * 7 + 2);
            buf.writeUInt16BE(slots[i].node, i * 7 + 4);
            buf.writeInt8(slots[i].option, i * 7 + 6);
      }
      return buf;
}

function post_init() {
      var coap_client = coap.request({
            hostname: "2001:0db8:1234:ffff:0000:00ff:fe00:0006",
            method: 'POST',
            confirmable: false,
            observe: false,
            pathname: '/harp_init',
            agent: new coap.Agent({ type: 'udp6' })
      });
      var buf = Buffer.alloc(8)
      buf.writeUInt8(1, 0)
      buf.writeUInt16LE(108, 1)
      buf.writeUInt8(1, 3)

      buf.writeUInt8(3, 4)
      // buf.writeUInt8(4, 5)
      // buf.writeUInt8(5, 6)

      coap_client.write(buf);
      coap_client.on("response", (log) => {
            console.log(log.payload.toString())
      })
      coap_client.on("error", (log) => {
            console.log(log)
      })
      // coap_client.write(buf)
      coap_client.end();
}

function post_iface() {
      var coap_client = coap.request({
            hostname: "2001:0db8:1234:ffff:0000:00ff:fe00:0003",
            method: 'POST',
            confirmable: false,
            observe: false,
            pathname: '/harp_iface',
            agent: new coap.Agent({ type: 'udp6' })
      });

      var payload = Buffer.alloc(25);
      payload.writeUInt8(1, 0)
      payload.writeUInt16LE(1, 1)
      payload.writeUInt8(1, 3)

      payload.writeUInt8(2, 4)
      payload.writeUInt16LE(1, 5)
      payload.writeUInt8(2, 7)

      payload.writeUInt8(3, 8)
      payload.writeUInt16LE(1, 9)
      payload.writeUInt8(2, 11)

      payload.writeUInt8(4, 12)
      payload.writeUInt16LE(1, 13)
      payload.writeUInt8(2, 15)

      console.log(payload);
      coap_client.on("response", (msg) => {
            console.log(msg.payload.toString())
      })
      coap_client.write(payload);
      coap_client.end();
}

function put_iface() {
      var coap_client = coap.request({
            hostname: "2001:0db8:1234:ffff:0000:00ff:fe00:0001",
            method: 'PUT',
            confirmable: false,
            observe: false,
            pathname: '/harp_iface',
            agent: new coap.Agent({ type: 'udp6' })
      });

      var payload = Buffer.alloc(4);
      payload.writeUInt8(1, 0)
      payload.writeUInt16LE(10, 1)
      payload.writeUInt8(1, 3)

      console.log(payload);
      coap_client.on("response", (msg) => {
            console.log(msg.payload.toString())
      })
      coap_client.write(payload);
      coap_client.end();
}

function post_sp() {
      var coap_client = coap.request({
            hostname: "2001:0db8:1234:ffff:0000:00ff:fe00:0003",
            method: 'POST',
            confirmable: false,
            observe: false,
            pathname: '/harp_sp',
            agent: new coap.Agent({ type: 'udp6' })
      });
      // l1-{17, 24, 3, 4}, l2-{11, 16, 3, 4}, l3-{8, 11, 4, 5}, l4-{6, 8, 1, 2},
      var payload = Buffer.alloc(42);
      var size = 0

      var layer = 0
      var ts_s = 0xffff
      var ts_e = 0xffff
      var ch_s = 0xff
      var ch_e = 0xff
      payload.writeUInt8(layer, size)
      size++
      payload.writeUInt16LE(ts_s, size)
      size+=2
      payload.writeUInt16LE(ts_e, size)
      size+=2
      payload.writeUInt8(ch_s, size)
      size++
      payload.writeUInt8(ch_e, size)
      size++

      var layer = 1
      var ts_s = 68
      var ts_e = 72
      var ch_s = 1
      var ch_e = 2
      payload.writeUInt8(layer, size)
      size++
      payload.writeUInt16LE(ts_s, size)
      size+=2
      payload.writeUInt16LE(ts_e, size)
      size+=2
      payload.writeUInt8(ch_s, size)
      size++
      payload.writeUInt8(ch_e, size)
      size++

      var layer = 2
      var ts_s = 65
      var ts_e = 68
      var ch_s = 1
      var ch_e = 2
      payload.writeUInt8(layer, size)
      size++
      payload.writeUInt16LE(ts_s, size)
      size+=2
      payload.writeUInt16LE(ts_e, size)
      size+=2
      payload.writeUInt8(ch_s, size)
      size++
      payload.writeUInt8(ch_e, size)
      size++

      var layer = 3
      var ts_s =  63
      var ts_e = 65
      var ch_s = 1
      var ch_e = 2
      payload.writeUInt8(layer, size)
      size++
      payload.writeUInt16LE(ts_s, size)
      size+=2
      payload.writeUInt16LE(ts_e, size)
      size+=2
      payload.writeUInt8(ch_s, size)
      size++
      payload.writeUInt8(ch_e, size)
      size++
      
      var layer = 4
      var ts_s = 62
      var ts_e = 63
      var ch_s = 1
      var ch_e = 2
      payload.writeUInt8(layer, size)
      size++
      payload.writeUInt16LE(ts_s, size)
      size+=2
      payload.writeUInt16LE(ts_e, size)
      size+=2
      payload.writeUInt8(ch_s, size)
      size++
      payload.writeUInt8(ch_e, size)
      size++

      var layer = 5
      var ts_s = 0xffff
      var ts_e = 0xffff
      var ch_s = 0xff
      var ch_e = 0xff
      payload.writeUInt8(layer, size)
      size++
      payload.writeUInt16LE(ts_s, size)
      size+=2
      payload.writeUInt16LE(ts_e, size)
      size+=2
      payload.writeUInt8(ch_s, size)
      size++
      payload.writeUInt8(ch_e, size)
      size++

      console.log(payload);
      coap_client.write(payload);
      coap_client.on("response", (msg) => {
            console.log(msg.payload.toString())
      })
      coap_client.end();
}

function get_iface() {
      var coap_client = coap.request({
            hostname: "2001:0db8:1234:ffff:0000:00ff:fe00:0009",
            method: 'GET',
            confirmable: false,
            observe: false,
            pathname: '/harp_iface',
            agent: new coap.Agent({ type: 'udp6' })
      });

      coap_client.on("response", (msg) => {
            console.log(msg.payload, msg.payload.length)
            idx = 0;
            for (var l = 0; l < msg.payload.length / 5; l++) {
                  layer = msg.payload.readUInt8(idx)
                  idx++
                  ts = msg.payload.readUInt16LE(idx)
                  idx += 2
                  ch = msg.payload.readUInt8(idx)
                  idx++
                  console.log(layer, ts, ch)
            }
      })
      coap_client.end();
}

function get_sp() {
      var id = 3
      var sp_list = []

      var coap_client = coap.request({
            hostname: "2001:0db8:1234:ffff:0000:00ff:fe00:" + id.toString(16),
            method: 'GET',
            confirmable: false,
            observe: false,
            pathname: '/harp_sp',
            agent: new coap.Agent({ type: 'udp6' })
      });

      coap_client.on("response", (msg) => {
            console.log(msg.payload)
            idx = 0;
            for (var l = 0; l < msg.payload.length / 7; l++) {
                  layer = msg.payload.readUInt8(idx)
                  idx++
                  ts_start = msg.payload.readUInt16LE(idx)
                  idx += 2
                  ts_end = msg.payload.readUInt16LE(idx)
                  idx += 2
                  ch_start = msg.payload.readUInt8(idx)
                  idx++
                  ch_end = msg.payload.readUInt8(idx)
                  idx++
                  console.log(layer, ts_start, ts_end, ch_start, ch_end)
                  sp_list.push({ layer: layer, ts_start: ts_start, ts_end: ts_end, ch_start: ch_start, ch_end: ch_end })
            }

            // var sp_data = {
            //       type: "harp_sp_data",
            //       gateway_0: cloud.data,
            //       msg: { _id: id, data: sp_list }
            // };
            // network_manager.sendDataToCloud(1, sp_data)
            // console.log(sp_data)
      })
      coap_client.end();
}

function get_sch() {
      var coap_client = coap.request(
            {
                  hostname: "2001:0db8:1234:ffff:0000:00ff:fe00:1",
                  method: 'GET',
                  confirmable: false,
                  observe: false,
                  pathname: '/schedule',
                  agent: new coap.Agent({ type: 'udp6' })
            });
      // var data = [];
      // startIdx = 0;
      // data[0] = (retrieved_schedule[id].seq) & 0xFF;
      // data[1] = startIdx & 0xFF;
      // data[2] = (startIdx >> 8) & 0xFF;  //LE format
      // coap_client.write(Buffer.from(data),'binary');
      coap_client.on("response", (msg) => {
            var tlv_data = Array.from(msg.payload);
            var sched_entry = tlv_to_schedule_params(tlv_data);
            console.log(msg.payload)
            console.log(sched_entry);
      })
      coap_client.end();
}

function put_sch() {
      var timeslot = 90
      var channel = 5
      var peer = 2
      var option = MAC_LINK_OPTION_TX

      var p2 = construct_payload([
            { timeslot: timeslot, channel: channel, node: peer, option: option }
      ]);
      var coap_client = coap.request(
            {
                  hostname: "2001:0db8:1234:ffff:0000:00ff:fe00:1",
                  method: 'PUT',
                  confirmable: false,
                  pathname: '/schedule',
                  agent: new coap.Agent({ type: 'udp6' })
            });
      coap_client.on('response', function (response) {
            console.log(response._packet)
      });
      coap_client.write(p2);
      coap_client.end();
}

function post_coap() {
      var coap_client = coap.request({
            hostname: "2001:0db8:1234:ffff:0000:00ff:fe00:0001",
            method: 'PUT',
            confirmable: true,
            observe: false,
            pathname: '/led',
            retrySend: 0,
            agent: new coap.Agent({ type: 'udp6' })
      });
      // var buf = Buffer.alloc(1)
      // buf.writeUInt8(0x25, 0)
      // buf.writeUInt8(0x21, 1)
      // buf.writeUInt8(0x21, 2)
      // buf.writeUInt16LE(1, 1)
      // buf.writeUInt8(1, 2)
      coap_client.write("1");
      coap_client.on("response", (log) => {
            console.log(log.payload.length,log.payload)
      })
      coap_client.on("error", (log) => {
            console.log(log)
      })
      // coap_client.write(buf)
      coap_client.end();
}

// get_sch()
// put_sch()
// post_init()
// post_iface()
put_iface()
// get_iface()
// post_sp()
// post_coap()
// get_sp()
