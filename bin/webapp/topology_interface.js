var coap = require('coap');
var coapTiming = {
  ackTimeout: 10,
  ackRandomFactor: 10,
  maxRetransmit: 10,
  maxLatency: 10,
  piggybackReplyMs: 10
};
function change(parent, nodev6){
  console.log("change parent command: "+parent+" to "+nodev6);
  var coap_client = coap.request({
    hostname: nodev6,
    method: 'PUT',
    confirmable: false,
    pathname: '/diagnosis',
    agent: new coap.Agent({ type: 'udp6'})
  });
  coap_client.on('response',function(){});
  coap_client.on('error', function()
  {
    console.log("totpology_intf change: error!");
    coap_client.freeCoapClientMemory();
  });
  coap_client.on('timeout', function()
  {
    console.log("totpology_intf change: timeout!");
    coap_client.freeCoapClientMemory();
  });
  coap_client.write(Buffer.from(parent.toString(), 'utf8'));
  coap_client.end();

}


module.exports={
  change:change
}
