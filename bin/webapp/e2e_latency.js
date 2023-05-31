var coap = require('coap')

var HCT_TLV_TYPE_LAST_SENT_ASN = 0x20
var HCT_TLV_TYPE_COAP_DATA_TYPE = 0x21
var HCT_TLV_TYPE_RECV_ASN = 0x22

var recv_asn = 0
var sent_asn = 0

var req1 = coap.request({
  hostname: "2001:0db8:1234:ffff:0000:00ff:fe00:000"+process.argv[2],
  method: 'GET',
  confirmable: false,
  observe: false,
  pathname: '/ping',
  agent: new coap.Agent({ type: 'udp6' })
});
req1.on('response',(resp1)=>{
    console.log("received from sender, forward to the receiver")
    var req2 = coap.request({
      hostname: "2001:0db8:1234:ffff:0000:00ff:fe00:000"+process.argv[3],
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
    req2.on('response',(resp2)=>{
      var ret = parse("receiver", resp2.payload)
      recv_asn = ret.received_asn
      console.log("rtt from device: "+recv_asn)
      
    })
    req2.end()
})
req1.end()

//req1.freeCoapClientMemory()
function parse(id, data) {
  var idx = 0;
  var remainLength = data.length;
  var obj = {
    received_asn: 0,
  }
  var dataType, dataLength;
  // console.log(data)
  while (remainLength > 0) {
    dataType = data.readUInt8(idx);
    idx += 1;
    dataLength = data.readUInt8(idx);
    idx += 1;
    switch (dataType) {
      case HCT_TLV_TYPE_RECV_ASN:
        obj.received_asn = data.readUInt16LE(idx);
        idx += 2;
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
  return obj
}
