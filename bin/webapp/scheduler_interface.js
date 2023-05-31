var coap = require('coap');
coap.updateTiming({
	ackTimeout: 20,
	ackRandomFactor: 5,
	maxRetransmit: 3,
	maxLatency: 30,
	piggybackReplyMs: 10
});


const MAC_LINK_OPTION_TX = 0x01
const MAC_LINK_OPTION_RX = 0x02
const RETRY_MAX = 3;
var beacon_allocation_tracker = {};

function construct_payload(slots){
  var buf = Buffer.alloc(slots.length*7)
  for (var i=0; i<slots.length; ++i){
    buf.writeUInt16BE(slots[i].timeslot, i*7+0);
    buf.writeUInt16BE(slots[i].channel,  i*7+2);
    buf.writeUInt16BE(slots[i].node,     i*7+4);
    buf.writeInt8(slots[i].option,       i*7+6);
  }
  return buf;
}

function do_add_tx_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, rx, tx, callback) 
{
   var ttime = new Date().getTime();   
   if (beacon_allocation_tracker[nodeTX] && beacon_allocation_tracker[nodeTX].addTxLink)  
      //these flags are needed to deal with the race condition  the node leaves and rejoins while the messaging is going on
   {
      var p2 = construct_payload([
         {timeslot: timeslot, channel:channel, node:nodeRX, option:MAC_LINK_OPTION_TX}
         ]);
      var coap_client2 = coap.request(
      {
         hostname: nodeTXv6,
         method: 'PUT',
         confirmable: false,
         pathname: '/schedule',
         agent: new coap.Agent({ type: 'udp6'})
      });
      coap_client2.on('response',function(response)
      {
         if (beacon_allocation_tracker[nodeTX] && (ttime - beacon_allocation_tracker[nodeTX].stime) > 0) //node has not left and rejoined since the transaction started
         {
            console.log("TX installed");
            if (callback) callback(nodeTX);
            if (rx)
            {
               add_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, RETRY_MAX, rx, false, callback);
            }
         }
         else
         {
            console.log("!!!!!!!!!do_add_tx_link, Node rejoined while CoAP trans in progress:", nodeTX);
         }
      });
   
      coap_client2.on('error',function()
      {
         coap_client2.freeCoapClientMemory();
         if (beacon_allocation_tracker[nodeTX] && (ttime - beacon_allocation_tracker[nodeTX].stime) > 0) //node has not left and rejoined since the transaction started
         {
            console.log("TX allocation failed");
            if (retry_count_down >= 0)
            {
               add_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, rx, true, callback)
            }
            else
            {
               if (beacon_allocation_tracker[nodeTX])
               {
                  beacon_allocation_tracker[nodeTX].addTxLink = null;
                  if (callback) callback(nodeTX, "TX link add failed");
               }
            }
         }
         else
         {
            console.log("!!!!!!!!!do_add_tx_link(err),Node rejoined while CoAP trans in progress:",nodeTX);
         }
      });
      
      coap_client2.on('timeout',function()
      {
         coap_client2.freeCoapClientMemory();
      });
   
      coap_client2.write(p2);
      coap_client2.end();
   }
}

function do_add_rx_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, rx, tx, callback)
{
   var ttime = new Date().getTime();   
   if (beacon_allocation_tracker[nodeRX] && beacon_allocation_tracker[nodeRX].addRxLink)
   {
      var p1 = construct_payload([
         {timeslot: timeslot, channel:channel, node:nodeTX, option:MAC_LINK_OPTION_RX}
         ]);
      
      var coap_client1 = coap.request(
      {
         hostname: nodeRXv6,
         method: 'PUT',
         confirmable: false,
         pathname: '/schedule',
         agent: new coap.Agent({ type: 'udp6'})
      });
      coap_client1.on('response',function(response)
      {
         if (beacon_allocation_tracker[nodeRX] && (ttime - beacon_allocation_tracker[nodeRX].stime) > 0) //node has not left and rejoined since the transaction started
         {
            console.log("RX installed");
            if (callback) callback(nodeRX);
            if (tx)
            {
               add_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, RETRY_MAX, false, tx, callback);
            }
         }
         else
         {
            console.log("!!!!!!!!!do_add_rx_link,Node rejoined while CoAP trans in progress:", nodeRX);
         }
      });
   
      coap_client1.on('error',function()
      {
         coap_client1.freeCoapClientMemory();
         if (beacon_allocation_tracker[nodeRX] && (ttime - beacon_allocation_tracker[nodeRX].stime) > 0) //node has not left and rejoined since the transaction started
         {
            console.log("RX allocation failed");
            if (retry_count_down >= 0)
            {
               add_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, true, tx, callback)
            }
            else
            {
               if (beacon_allocation_tracker[nodeRX])
               {
                  beacon_allocation_tracker[nodeRX].addRxLink = null;
                  if (callback) callback(nodeRX, "RX link add failed");
               }
            }
         }
         else
         {
            console.log("!!!!!!!!!do_add_rx_link(err),Node rejoined while CoAP trans in progress:", nodeRX);
         }
      });
      
      coap_client1.on('timeout',function()
      {
         coap_client1.freeCoapClientMemory();
      });
   
      coap_client1.write(p1);
      coap_client1.end();
   }
}

function add_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, rx, tx, callback)
{
   if(typeof sim !== 'undefined'&&sim==1)return;
   console.log("add_link("+timeslot+","+channel+","+nodeTX+","+nodeRX+","+nodeTXv6+","+nodeRXv6+")");
   if (retry_count_down == null)
   {
      retry_count_down = RETRY_MAX;
   }
   
   if (rx == null)
   {
      rx = true;
   }
   
   if (tx == null)
   {
      tx = true;
   }
   
   if (retry_count_down >= 0)
   {
      retry_count_down--;
   
      if (rx)
      {
         do_add_rx_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, rx, tx, callback);         
      }
      else if (tx)
      {
         do_add_tx_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, rx, tx, callback);         
      }
   }
}

function do_rm_tx_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, rx, tx, callback)
{
   var ttime = new Date().getTime();  
   if (beacon_allocation_tracker[nodeTX] && beacon_allocation_tracker[nodeTX].rmTxLink)
   {
      var p1 = construct_payload([
         {timeslot: timeslot, channel:channel, node:nodeRX, option:MAC_LINK_OPTION_TX}
         ]);
   
      var coap_client1 = coap.request(
      {
         hostname: nodeTXv6,
         method: 'DELETE',
         confirmable: false,
         pathname: '/schedule',
         agent: new coap.Agent({ type: 'udp6'})
      });
      coap_client1.on('response',function(response)
      {
         if (beacon_allocation_tracker[nodeTX] && (ttime - beacon_allocation_tracker[nodeTX].stime) > 0) //node has not left and rejoined since the transaction started
         {
            console.log("TX deleted");
            if (callback) callback(nodeTX);
            if (rx)
            {
               rm_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, RETRY_MAX, rx, false, callback);
            }
         }
         else
         {
            console.log("!!!!!!!!!do_rm_tx_link,Node left or rejoined while CoAP trans in progress:", nodeTX);
         }
      });
      
      coap_client1.on('error',function()
      {
         coap_client1.freeCoapClientMemory();
         if (beacon_allocation_tracker[nodeTX] && (ttime - beacon_allocation_tracker[nodeTX].stime) > 0) //node has not left and rejoined since the transaction started
         {
            console.log("TX link removal failed");
            if (retry_count_down >= 0)
            {
               rm_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, rx, true, callback)
            }
            else
            {
               if (beacon_allocation_tracker[nodeTX])
               {
                  beacon_allocation_tracker[nodeTX].rmTxLink = null;
                  if (callback) callback(nodeTX, "TX link removal failed");
               }
            }
         }
         else
         {
            console.log("!!!!!!!!!do_rm_tx_link(err),Node left or rejoined while CoAP trans in progress:", nodeTX);
         }
      });
      
      coap_client1.on('timeout',function()
      {
         coap_client1.freeCoapClientMemory();
      });
      coap_client1.write(p1);
      coap_client1.end();
   }
}

function do_rm_rx_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, rx, tx, callback)
{
   var ttime = new Date().getTime();
   if (beacon_allocation_tracker[nodeRX] && beacon_allocation_tracker[nodeRX].rmRxLink)
   {
      var p2 = construct_payload([
      {timeslot: timeslot, channel:channel, node:nodeTX, option:MAC_LINK_OPTION_RX}
      ]);
      var coap_client2 = coap.request(
      {
         hostname: nodeRXv6,
         method: 'DELETE',
         confirmable: false,
         pathname: '/schedule',
         agent: new coap.Agent({ type: 'udp6'})
      });
      coap_client2.on('response',function(response)
      {
         if (beacon_allocation_tracker[nodeRX] && (ttime - beacon_allocation_tracker[nodeRX].stime) > 0) //node has not left and rejoined since the transaction started
         {
            console.log("RX deleted");
            if (callback) callback(nodeRX);
            if (tx)
            {
               rm_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, RETRY_MAX, false, tx, callback);
            }
         }
         else
         {
            console.log("!!!!!!!!!do_rm_rx_link, Node rejoined while CoAP trans in progress:", nodeRX);
         }
      });
      
      coap_client2.on('error',function()
      {
         coap_client2.freeCoapClientMemory();
         if (beacon_allocation_tracker[nodeRX] && (ttime - beacon_allocation_tracker[nodeRX].stime) > 0) //node has not left and rejoined since the transaction started
         {
            console.log("RX link removal failed");
            if (retry_count_down >= 0)
            {
               rm_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, true, tx, callback)
            }
            else
            {
               if (beacon_allocation_tracker[nodeRX])
               {
                  beacon_allocation_tracker[nodeRX].rmRxLink = null;
                  if (callback) callback(nodeRX, "RX link removal failed");
               }
            }
         }
         else
         {
            console.log("!!!!!!!!!do_rm_rx_link(err), Node rejoined while CoAP trans in progress:", nodeRX);
         }
      });
      
      coap_client2.on('timeout',function()
      {
         coap_client2.freeCoapClientMemory();
      });
      
      coap_client2.write(p2);
      coap_client2.end();
   }
}

function rm_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, rx, tx, callback)
{
   if(typeof sim !== 'undefined'&&sim==1)return;
   if (retry_count_down == null)
   {
      retry_count_down = RETRY_MAX;
   }
   
   if (rx == null)
   {
      rx = true;
   }
   
   if (tx == null)
   {
      tx = true;
   }
   
   console.log("rm_link("+timeslot+","+channel+","+nodeTX+","+nodeRX+","+nodeTXv6+","+nodeRXv6+")");
   if (retry_count_down >= 0)
   {
      retry_count_down--;
   
      if (rx)
      {
         do_rm_rx_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, rx, tx, callback);         
      }
      else if (tx)
      {
         do_rm_tx_link(timeslot, channel, nodeTX, nodeRX, nodeTXv6, nodeRXv6, retry_count_down, rx, tx, callback);         
      }
   }
}

/*add_link(60, 0, 1, 3, "2001:0db8:1234:ffff:0000:00ff:fe00:0001", "2001:0db8:1234:ffff:0000:00ff:fe00:0003")
add_link(61, 0, 3, 1, "2001:0db8:1234:ffff:0000:00ff:fe00:0003", "2001:0db8:1234:ffff:0000:00ff:fe00:0001")
add_link(30, 0, 1, 3, "2001:0db8:1234:ffff:0000:00ff:fe00:0001", "2001:0db8:1234:ffff:0000:00ff:fe00:0003")
add_link(31, 0, 3, 1, "2001:0db8:1234:ffff:0000:00ff:fe00:0003", "2001:0db8:1234:ffff:0000:00ff:fe00:0001")
add_link(90, 0, 1, 3, "2001:0db8:1234:ffff:0000:00ff:fe00:0001", "2001:0db8:1234:ffff:0000:00ff:fe00:0003")
add_link(91, 0, 3, 1, "2001:0db8:1234:ffff:0000:00ff:fe00:0003", "2001:0db8:1234:ffff:0000:00ff:fe00:0001")*/

module.exports={
  add_link:add_link,
  rm_link:rm_link,
  beacon_allocation_tracker:beacon_allocation_tracker,
  do_add_tx_link: do_add_tx_link,
  do_add_rx_link: do_add_rx_link
  
}
