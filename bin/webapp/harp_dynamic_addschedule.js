var coap = require('coap')

function id2address(id) {
   return "2001:db8:1234:ffff::ff:fe00:" + id.toString(16);
}

function construct_payload(slots){
  var buf = Buffer.alloc(slots.length*7);
  for (var i=0; i<slots.length; ++i){
    buf.writeUInt16BE(slots[i].timeslot+0, i*7+0);
    buf.writeUInt16BE(slots[i].channel,  i*7+2);
    buf.writeUInt16BE(slots[i].node,     i*7+4);
    buf.writeInt8(slots[i].option,       i*7+6);
  }
  return buf;
}

const MAC_LINK_OPTION_TX = 0x01
const MAC_LINK_OPTION_RX = 0x02
const MAC_LINK_OPTION_COAP_TX = 0x21

// var timeslot = process.argv[2]
// var channel = process.argv[3]
// var node = process.argv[4]
// var peer = process.argv[5]
// var option = process.argv[6]



var coap_client1 = coap.request(
{
   hostname: id2address(1),
   method: 'PUT',
   confirmable: false,
   pathname: '/schedule',
   agent: new coap.Agent({ type: 'udp6'})
});
coap_client1.on('response',function(response)
{
	console.log(response._packet)
});
// to gateway
var p1 = construct_payload([
  // {timeslot: 30, channel:1, node:3, option:0x02},
  // {timeslot: 180, channel:1, node:3, option:0x21}
  {timeslot: 40, channel:1, node:3, option:0x02},
  {timeslot: 170, channel:1, node:3, option:0x21},
  {timeslot: 50, channel:1, node:3, option:0x02},
  {timeslot: 60, channel:1, node:3, option:0x02},
  {timeslot: 70, channel:1, node:3, option:0x02},
  {timeslot: 140, channel:1, node:3, option:0x21},
  {timeslot: 150, channel:1, node:3, option:0x21},
  {timeslot: 160, channel:1, node:3, option:0x21},


]);
coap_client1.write(p1);
coap_client1.end();


// to device
var coap_client2 = coap.request(
  {
     hostname: id2address(3),
     method: 'PUT',
     confirmable: false,
     pathname: '/schedule',
     agent: new coap.Agent({ type: 'udp6'})
  });
  coap_client2.on('response',function(response)
  {
    console.log(response._packet)
  });
  // to gateway
  var p2 = construct_payload([
    // {timeslot: 30, channel:1, node:1, option:0x21},
    // {timeslot: 180, channel:1, node:1, option:0x02} // 1000
    {timeslot: 40, channel:1, node:1, option:0x21},
    {timeslot: 170, channel:1, node:1, option:0x02},
    {timeslot: 50, channel:1, node:1, option:0x21},
    {timeslot: 60, channel:1, node:1, option:0x21},
    {timeslot: 70, channel:1, node:1, option:0x21},
    {timeslot: 140, channel:1, node:1, option:0x02},
    {timeslot: 150, channel:1, node:1, option:0x02},
    {timeslot: 160, channel:1, node:1, option:0x02},

    
  ]);
  coap_client2.write(p2);
  coap_client2.end();