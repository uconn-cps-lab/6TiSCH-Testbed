var coap = require('coap')

function id2address(id) {
   return "2001:db8:1234:ffff::ff:fe00:" + id.toString(16);
}

function construct_payload(slots){
  var buf = Buffer.alloc(slots.length*7);
  for (var i=0; i<slots.length; ++i){
    buf.writeUInt16BE(slots[i].timeslot, i*7+0);
    buf.writeUInt16BE(slots[i].channel,  i*7+2);
    buf.writeUInt16BE(slots[i].node,     i*7+4);
    buf.writeInt8(slots[i].option,       i*7+6);
  }
  return buf;
}

const LINK_OPTION_TX_MGMT = 0x01
const LINK_OPTION_TX_COAP = 0x21
const LINK_OPTION_RX = 0x0A
const LINK_OPTION_ADV = 0x09

var timeslot = process.argv[2]
var channel = process.argv[3]
var node = process.argv[4]
var peer = process.argv[5]
var option = process.argv[6]

var p2 = construct_payload([
   {timeslot: timeslot, channel:channel, node:peer, option:option}
   ]);
var coap_client2 = coap.request(
{
   hostname: id2address(node),
   method: 'PUT',
   confirmable: false,
   pathname: '/schedule',
   agent: new coap.Agent({ type: 'udp6'})
});
coap_client2.on('response',function(response)
{
	console.log(response._packet)
});

coap_client2.on('error',function()
{
});

coap_client2.on('timeout',function()
{
   coap_client2.freeCoapClientMemory();
});

coap_client2.write(p2);
coap_client2.end();

