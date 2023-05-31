var coap = require("coap");
var id=+process.argv[2];
var startIdx = +process.argv[3];
function id2address(id){
  return "2001:db8:1234:ffff::ff:fe00:"+id.toString(16);
}
function tlv_to_schedule_params(tlvdata)
{

   var sched_entry = {seq: 0, totalLinkCount: 0, retrievedLinkCount: 0, startLinkIdx: 0, links: []};

   if (tlvdata.length >= 4)
   {
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
      
      while (idx < tlvdata.length)
      {        

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
         link.link_id = tlvdata[idx] + tlvdata[idx+1]*256;  //LE format
         idx+=2;  
         
         if ((tlvdata.length - idx) < 2) break;
         link.timeslot = tlvdata[idx] + tlvdata[idx+1]*256;  //LE format
         idx+=2;  
         
         if ((tlvdata.length - idx) < 2) break;
         link.channel_offset = tlvdata[idx] + tlvdata[idx+1]*256;  //LE format
         idx+=2;  
         
         if ((tlvdata.length - idx) < 2) break;
         link.peer_addr = tlvdata[idx] + tlvdata[idx+1]*256;  //LE format
         idx+=2;  
         
         linkCount++;        
      }
      
      if (linkCount != sched_entry.retrievedLinkCount || linkCount > sched_entry.totalLinkCount)
      {
         sched_entry.totalLinkCount = 0;
         sched_entry.retrievedLinkCount = 0;
      }
   }

   return sched_entry;
}
var coap_client = coap.request(
{
	hostname: id2address(id),
	method: 'GET',
	confirmable: false,
	observe: false,
	pathname: '/schedule',
	agent: new coap.Agent({ type: 'udp6'})
});

var data = [];
data[0] = 0xFF;
data[1] = startIdx & 0xFF;
data[2] = (startIdx >> 8) & 0xFF;  //LE format
coap_client.write(Buffer.from(data),'binary');

console.log("retrieve_schedule():"+id+","+startIdx.toString());

coap_client.on('response', function(msg)
{
	var tlv_data = Array.from(msg.payload);
	var sched_entry = tlv_to_schedule_params(tlv_data);
	console.log(sched_entry);
});

coap_client.on('error', function()
{
	console.log("Schedule retrieve from node "+id+" error!");
});

coap_client.on('timeout', function()
{
	console.log("Schedule retrieve from node "+id+" timeout!");
});
							 
coap_client.end();   
