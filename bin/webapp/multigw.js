/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
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
//================================================================================
// MODULES AND GLOBALS
//================================================================================
var http           = require('http');
//================================================================================
const MAX_VPAN_SIZE = 100;
const MAX_PEER_EXCHANGE_ERROR_COUNT = 1;
//======================================================
//Globals:
//======================================================
 
//======================================================
//Locals:
//======================================================
const node_address_span = 1000;
const COORD_KA_COUNTDOWN = 2;
const MAX_DEPARTED_PEER_CONTACT_COUNT = 200;  //keep contacting for two hours
const VPAN_START_TIMEOUT = 120000;   //VPAN start up max time: 120 seconds
const VPAN_TRANSAC_TIMEOUT = 3000;   //VPAN peer http transaction timeout: 3 seconds
var coordinatorKeepAliveCountDown = COORD_KA_COUNTDOWN;
var vpanStartTimer;
var ownVpanStarted = false;
var gw_id = 1;
var vpan = 
{
   'isActive':false,
   'vpanId':0, 
   'coordinator':'', 
   'peers':[], 
   'departed_peers':[], 
   'frontEndServerList':[], 
   'node_address_base':0,
   'nodes':[]
};  
//======================================================
//Module HOOKS to be provided by the system:
//======================================================
var settings = {};
var httpPort = 80;
var web_broadcast;
var http_get;
var http_put;
var db_find_nwk_nui_nodes;
var db_find_groups;
var db_node_count;
var db_node_find;
var init_callback;
var get_node_id_callback;
var get_nodes_callback;
//======================================================
//functions:
//======================================================
function initCode1()
{
   if(vpan.isActive)
   {
      start_ka_timer();
   }
   if (init_callback != null)
   {
      init_callback();
   }
}
//======================================================
function getRandomInt(min, max)
{
   return Math.floor(Math.random() * (max - min + 1)) + min;
}
//======================================================
function ipv6AddrToipv4Addr(ipv6Addr)
{
   var remoteAddr = ipv6Addr;
   var idx = remoteAddr.lastIndexOf(":");
   if (idx >= 0)
   {
      remoteAddr = remoteAddr.substring(idx + 1);
   }
     
   return remoteAddr;
}
//======================================================
function vpanIsActive()
{
   return(vpan.isActive);
}
//======================================================
function vpanId()
{
   return(vpan.vpanId);
}
//======================================================
function vpanNodeAddressBase()
{
   return(vpan.node_address_base);
}
//======================================================
function isGateway(vapnAddr)
{
   return (vapnAddr % node_address_span==1);
}
//======================================================
function panAddrTovpanAddr(panAddr)
{
   var vpanAddr = panAddr + vpan.node_address_base;
   if (vpan.isActive)
   {
      for (var i = 0; i < vpan.nodes.length; i++)
      {
         if (vpan.nodes[i].panAddr == panAddr && vpan.nodes[i].gw == vpan.node_address_base)
         {
            vpanAddr = vpan.nodes[i].vpanAddr;
            break;
         }
      }
   }
   else
   {
      vpanAddr = panAddr;
   }
   
   return vpanAddr;
}
//======================================================
function vpanAddrTopanAddr(vpanAddr)
{
   var panAddr = vpanAddr - vpan.node_address_base;
   if (vpan.isActive)
   {
      for (var i = 0; i < vpan.nodes.length; i++)
      {
         if (vpan.nodes[i].vpanAddr == vpanAddr)
         {
            panAddr = vpan.nodes[i].panAddr;
            break;
         }
      }
   }
   else
   {
      panAddr = vpanAddr;
   }
   
   return panAddr;
}
//======================================================
function vpanAddrToGw(vpanAddr)
{
   var gw = -1;
   if (vpan.isActive)
   {
      for (var i = 0; i < vpan.nodes.length; i++)
      {
         if (vpan.nodes[i].vpanAddr == vpanAddr)
         {
            gw = vpan.nodes[i].gw;
            break;
         }
      }
   }
   else
   {
      gw = vpan.node_address_base;
   }
    
   return gw;
}
//================================================================================
function inform_vpan_peers_frontend_connected()
{  
   console.log("inform_vpan_peers_frontend_connected()");
   if (vpan && vpan.peers && vpan.peers.length > 0)
   {
      for (var i = 0; i < vpan.peers.length; i++)
      {
         informNextPeerFrontEndConnected(vpan.peers[i].ip_address, 0);
      }
   }
}
//======================================================
function informNextPeerFrontEndConnected(ip_address, errorCount)
{
   var addr = ip_address;
   var ec = errorCount;
   
   //console.log("informNextPeerFrontEndConnected():" + ip_address);
   var transacActive = true;
   var transacTimer = setTimeout(function()
   {
      transacActive = false;
      vapanPeerDepartedHandler(addr);
   }, VPAN_TRANSAC_TIMEOUT);    
   
   const options = 
   {
      hostname: ip_address,
      port: httpPort,
      path: '/frontendconnected?ip_address='+settings.vpan.my_url,
      method: 'PUT',
   };
   
   //console.log("PUT to:"+options.hostname+':'+options.port+'/'+options.path);

   const req = http.request(options, function(res)
   {
      //console.log(res.statusCode);
      res.setEncoding('utf8');
      res.on('data', function(chunk)
      {
         //console.log('Chunk:'+chunk);
      });
      
      res.on('end', function()
      {
         if (transacActive)
         {
            clearTimeout(transacTimer);
         }
      });
   });

   req.on('error', function(e) 
   {
      if (transacActive)
      {
         clearTimeout(transacTimer);
         ec++;
         console.error(`problem with request (F): ${e.message}:`+ec);
   
         if (ec > MAX_PEER_EXCHANGE_ERROR_COUNT)
         {
            vapanPeerDepartedHandler(addr);
         }
         else
         {
            informNextPeerFrontEndConnected(addr, ec);  //Retry
         }
      }
   });
   req.end(); 
}
//================================================================================
function inform_vpan_peers_frontend_disconnected()
{  
   console.log("inform_vpan_peers_frontend_disconnected()");
   if (vpan && vpan.peers && vpan.peers.length > 0)
   {
      for (var i = 0; i < vpan.peers.length; i++)
      {
         informNextPeerFrontEndDisconnected(vpan.peers[i].ip_address, 0);
      }
   }
}
//======================================================
function informNextPeerFrontEndDisconnected(ip_address, errorCount)
{
   var addr = ip_address;
   var ec = errorCount;
   
   //console.log("informNextPeerFrontEndDisconnected():" + ip_address);
   var transacActive = true;
   var transacTimer = setTimeout(function()
   {
      transacActive = false;
      vapanPeerDepartedHandler(addr);
   }, VPAN_TRANSAC_TIMEOUT); 
   
   const options = 
   {
      hostname: ip_address,
      port: httpPort,
      path: '/frontenddisconnected?ip_address='+settings.vpan.my_url,
      method: 'PUT',
   };
   
   //console.log("PUT to:"+options.hostname+':'+options.port+'/'+options.path);

   const req = http.request(options, function(res)
   {
      //console.log(res.statusCode);
      res.setEncoding('utf8');
      res.on('data', function(chunk)
      {
         //console.log('Chunk:'+chunk);
      });
      
      res.on('end', function()
      {
         if (transacActive)
         {
            clearTimeout(transacTimer);
         }
      });
   });

   req.on('error', function(e) 
   {
      if (transacActive)
      {
         clearTimeout(transacTimer);
         ec++;
         console.error(`problem with request (G): ${e.message}:`+ec);
   
         if (ec > MAX_PEER_EXCHANGE_ERROR_COUNT)
         {
            vapanPeerDepartedHandler(addr);
         }
         else
         {
            informNextPeerFrontEndDisconnected(addr, ec);  //Retry
         }
      }
   });
   req.end();
}
//================================================================================
function sendFrontEndDataToServer(ip_address, data, errorCount)
{
   var addr = ip_address;
   var fdata = data;
   var ec = errorCount;
   var transacActive = true;
   var transacTimer = setTimeout(function()
   {
      transacActive = false;
      frontEndServerListRemove(addr);
   }, VPAN_TRANSAC_TIMEOUT); 
   
   const options = 
   {
      hostname:  ip_address,
      port: httpPort,
      path: '/frontenddata?fdata='+fdata,
      method: 'PUT',
   };
      
   //console.log("PUT to:"+options.hostname+':'+options.port+'/'+options.path);
   var req = http.request(options, function(res)
   {
      //console.log(res.statusCode);
      res.setEncoding('utf8');
      res.on('data', function(chunk)
      {
         //console.log('Chunk:'+chunk);
      });
      
      res.on('end', function()
      {
         if (transacActive)
         {
            clearTimeout(transacTimer);
         }
      });
   });
   
   req.on('error', function(e) 
   {
      if (transacActive)
      {
         clearTimeout(transacTimer);
         ec++;
         console.error(`problem with request (A): ${e.message}:`+ec);
         if (ec < MAX_PEER_EXCHANGE_ERROR_COUNT)
         {
            sendFrontEndDataToServer(addr, fdata, ec); //retry
         }
         else
         {
            frontEndServerListRemove(addr);
         }
      }
   });
   req.end(); 
}
//======================================================
function broadcastFrontEndDataToServers(fdata)
{
   for (var i = 0; i < vpan.frontEndServerList.length; i++)
   {
      //console.log("broadcastFrontEndDataToServers():" + vpan.frontEndServerList[i].ip_address);  
      sendFrontEndDataToServer(vpan.frontEndServerList[i].ip_address, fdata, 0);
   }
}
//======================================================
function vpan_adjust_ipaddress(mstruct)
{
   if (vpan.isActive)
   {
      if (mstruct.address && mstruct._id)
      {     
         //console.log('vpan_adjust_ipaddress() in:'+mstruct.address);
         var idx = mstruct.address.lastIndexOf(':');
         if (idx >= 0)
         {
            var saddrStr = mstruct._id.toString(16);
            while (saddrStr.length < 4)
            {
               saddrStr = '0'+saddrStr;
            }
            mstruct.address = mstruct.address.slice(0,idx+1) + saddrStr;      
         }
         //console.log('vpan_adjust_ipaddress() out:'+mstruct.address);
      }
   }
}
//======================================================
function assignVpanAddress(oldShortAddr, node_eui64)
{
   var vpanAddr = oldShortAddr;

   if (vpan.isActive)
   {
      var newNode = {
            eui64:node_eui64, 
            panAddr:0, 
            gw:vpan.node_address_base, 
            vpanAddr:0
         };
      
      var i = nodeListSearch(newNode);
      if (i >= 0)
      {
         vpanAddr = newNode.panAddr = newNode.vpanAddr = vpan.nodes[i].vpanAddr;
      }
      else
      {
         vpanAddr = newNode.panAddr = newNode.vpanAddr = oldShortAddr + vpan.node_address_base;
      }
  
      nodeListAdd(newNode);
      vpanInformNodeJoin(newNode);
   }
   
   return vpanAddr;
}
//======================================================
function vpan_adjust_params(mstruct)
{
   if (vpan.isActive)
   {
      if (mstruct)
      {
         if (mstruct._id)
         {
            mstruct._id = panAddrTovpanAddr(mstruct._id);
         }
         if (mstruct.parent)
         {
            mstruct.parent = panAddrTovpanAddr(mstruct.parent );
         }
         
         if (mstruct.retrived_schedule && mstruct.retrived_schedule.links)
         {
            for (var j = 0; j < mstruct.retrived_schedule.links.length; j++)
            {
               if (mstruct.retrived_schedule.links[j].peer_addr != 65535)
               {
                  mstruct.retrived_schedule.links[j].peer_addr = 
                     panAddrTovpanAddr(mstruct.retrived_schedule.links[j].peer_addr);
               }
            }
         } 
         
         vpan_adjust_ipaddress(mstruct);
      }
   }
}
//======================================================
function vpan_restore_params(mstruct)
{
   if (vpan.isActive)
   {
      if (mstruct)
      {
         if (mstruct._id)
         {
            mstruct._id = vpanAddrTopanAddr(mstruct._id);
         }
         if (mstruct.parent)
         {
            mstruct.parent = vpanAddrTopanAddr(mstruct.parentd);
         }
         
         if (mstruct.retrived_schedule && mstruct.retrived_schedule.links)
         {
            for (var j = 0; j < mstruct.retrived_schedule.links.length; j++)
            {
               if (mstruct.retrived_schedule.links[j].peer_addr != 65535)
               {
                  mstruct.retrived_schedule.links[j].peer_addr = 
                     vpanAddrTopanAddr(mstruct.retrived_schedule.links[j].peer_addr);
               }
            }
         }
         
         vpan_adjust_ipaddress(mstruct);
      }
   }
}
//======================================================
function peerListSearch(ip_address)
{
   var retVal = -1;
   
   if (vpan.isActive)
   {  
      for (var i = 0; i < vpan.peers.length; i++)
      {
         if (vpan.peers[i].ip_address == ip_address)
         {
            retVal = i;
            break;
         }
      }
   }
   return retVal;
}
//======================================================
function peerListRemove(ip_address)
{
   var idx = peerListSearch(ip_address);
   if (idx >= 0)
   {
      vpan.peers.splice(idx, 1);     
   }
   
   frontEndServerListRemove(ip_address);
}
//======================================================
function peerListAdd(newPeer)
{
   var success = true;
   if (vpan.isActive)
   {
      var ip_address = newPeer.ip_address;
      if (ip_address != settings.vpan.my_url)
      {
         var idx = departedPeerListSearch(ip_address);
         if (idx >= 0)
         {
            vpan.departed_peers[idx].node_address_base = newPeer.node_address_base;
         }
         
         idx = peerListSearch(ip_address);
         if (idx < 0)
         {
            if (vpan.peers.length >= MAX_VPAN_SIZE)
            {
               success = false;
            }
            else
            {        
               vpan.peers.push(newPeer);
               departedPeerListRemove(ip_address);
            }
         }
         else
         {
            vpan.peers[idx].node_address_base = newPeer.node_address_base;
         }
      }
   }
   return success;
}
//======================================================
function departedPeerListSearch(ip_address)
{
   var retVal = -1;
   if (vpan.isActive)
   {
      for (var i = 0; i < vpan.departed_peers.length; i++)
      {
         if (vpan.departed_peers[i].ip_address == ip_address)
         {
            retVal = i;
            break;
         }
      }
   }
   return retVal;
}
//======================================================
function departedPeerListRemove(ip_address)
{
   var idx = departedPeerListSearch(ip_address);
   if (idx >= 0)
   {
      vpan.departed_peers.splice(idx, 1);     
   }
}
//======================================================
function departedPeerListAdd(departedPeer)
{
   var success = true;
   if (vpan.isActive)
   {
      var ip_address = departedPeer.ip_address;
      if (ip_address != settings.vpan.my_url)
      {
         var idx = departedPeerListSearch(ip_address);
         if (idx < 0)
         {
            if (vpan.departed_peers.length >= MAX_VPAN_SIZE)
            {
               vpan.departed_peers.splice(0,1);  //delete the oldest departed peer record
            }
   
            vpan.departed_peers.push({'ip_address':ip_address, 'node_address_base':departedPeer.node_address_base, 'contactCountDown':MAX_DEPARTED_PEER_CONTACT_COUNT});
         }
               
         peerListRemove(ip_address);
      }
   }
   return success;
}
//======================================================
function departedPeerComeBackCheck(ip_address)
{
   var retVal = false;
   var idx = departedPeerListSearch(ip_address);
   if (idx >= 0)
   {
      peerListAdd(vpan.departed_peers[idx]);
      retVal = true;
   }
   return retVal;
}
//======================================================
function frontEndServerListSearch(ip_address)
{
   var retVal = -1;
   if (vpan.isActive)
   {
      for (var i = 0; i < vpan.frontEndServerList.length; i++)
      {
         if (vpan.frontEndServerList[i].ip_address == ip_address)
         {
            retVal = i;
            break;
         }
      }
   }
   return retVal;
}
//======================================================
function frontEndServerListRemove(ip_address)
{
   var idx = frontEndServerListSearch(ip_address);
   if (idx >= 0)
   {
      vpan.frontEndServerList.splice(idx, 1);     
   }
}
//======================================================
function frontEndServerListAdd(peer)
{
   if (vpan.isActive)
   {
      if (peerListSearch(peer.ip_address) >= 0)
      {
         var idx = frontEndServerListSearch(peer.ip_address);
         if (idx < 0)
         {
            vpan.frontEndServerList.push(peer);
         }
      }
   }
}
//================================================================================
function nodeListSearch(nn)
{
   var retVal = -1;
   if (vpan.isActive)
   {
      for (var i = 0; i < vpan.nodes.length; i++)
      {
         if (vpan.nodes[i].eui64 == nn.eui64)
         {
            retVal = i;
            break;
         }
      }
   }
   
   return retVal;
}
//======================================================
function nodeListAdd(nn)
{
   if (vpan.isActive)
   {
      var idx = nodeListSearch(nn)
      if (idx >= 0)
      {
         vpan.nodes[idx].panAddr = nn.panAddr;
         vpan.nodes[idx].gw = nn.gw;
         nn.vpanAddr = vpan.nodes[idx].vpanAddr;  //vpanAddr doesn't change
      }
      else
      {      
         vpan.nodes.push(nn);        
      }
   }
}
//======================================================
function vpanNodesListPurge(nodeList)
{
   if (vpan.isActive)
   {
      //Remove all the nodes that are known to be connected to the peer gateways at this time
      for (var i = 0; i < nodeList.length; i++)
      {
         var idx = nodeListSearch({eui64:nodeList[i].eui64});
         if (idx >= 0 && vpan.nodes[idx].gw != vpan.node_address_base)
         {
            nodeList.splice(i, 1);
            i--;
         }
      }
   }
}
//================================================================================
function vpanInformCoordinator(ip_address)
{
   var addr = ip_address;
   const options = 
   {
      hostname: ip_address,
      port: httpPort,
      path: '/coordinator?ip_address='+vpan.coordinator,
      method: 'PUT',
   };
         
   console.log('vpanInformCoordinator():'+ip_address);
   //console.log("PUT to:"+options.hostname+':'+options.port+'/'+options.path);

   const req = http.request(options, function(res)
   {
      //console.log(res.statusCode);
      res.setEncoding('utf8');
      res.on('data', function(chunk)
      {
      });
      
      res.on('end', function()
      {
      });
   });

   req.on('error', function(e) 
   {  
   });
   req.end(); 
}
//======================================================
function vpanInformPeerIamStillHere(ip_address)
{
   const options = 
   {
      hostname: ip_address,
      port: httpPort,
      path: '/iamstillhere',
      method: 'PUT',
   };
   
   const req = http.request(options, function(res)
   {
      //console.log(res.statusCode);
      res.setEncoding('utf8');
      res.on('data', function(chunk)
      {
      });
      
      res.on('end', function()
      {
      });
   });

   req.on('error', function(e) 
   {
   });
   req.end();
}
//======================================================
function informPeerIamCoordinator()
{
   vpan.coordinator = settings.vpan.my_url;
   for (var i = 0; i < vpan.peers.length; i++)
   {
      vpanInformCoordinator(vpan.peers[i].ip_address);
   }
   
   for (var i = 0; i < vpan.departed_peers.length; i++)
   {
      if (vpan.departed_peers[i].contactCountDown > 0)
      {
         vpan.departed_peers[i].contactCountDown--;
         vpanInformCoordinator(vpan.departed_peers[i].ip_address);  //try to bring the departed ones back
      }
   }
}
//======================================================
function vpanInformIamStillHere()
{
   for (var i = 0; i < vpan.peers.length; i++)
   {  
      vpanInformPeerIamStillHere(vpan.peers[i].ip_address);
   }
   
   for (var i = 0; i < vpan.departed_peers.length; i++)
   {  
      if (vpan.departed_peers[i].contactCountDown > 0)
      {
         vpan.departed_peers[i].contactCountDown--;
         vpanInformPeerIamStillHere(vpan.departed_peers[i].ip_address);  //Try to bring the departed ones back
      }
   }
}
//======================================================
function start_ka_timer()
{
   setTimeout(function()
   {
      if (!iamTheCoordinator() && coordinatorKeepAliveCountDown == 0)
      {
         vpan.coordinator = settings.vpan.my_url;
      }

      if (iamTheCoordinator())
      {
         informPeerIamCoordinator();
      }
      else
      {
         vpanInformIamStillHere();
      }
      
      coordinatorKeepAliveCountDown--;
      start_ka_timer();
   }, 30000+getRandomInt(1, 10000));    
}
//======================================================
function vapanPeerDepartedHandler(ip_address)
{
   var peeridx = peerListSearch(ip_address);
   if (peeridx >= 0)
   {     
      if (vpan.peers[peeridx].ip_address == vpan.coordinator)
      {
         console.log("Coordinator departed:"+vpan.peers[peeridx].ip_address);
         vpan.coordinator = settings.vpan.my_url;      
      }
      else
      {
         console.log("Peer departed:"+vpan.peers[peeridx].ip_address);   
      }
      departedPeerListAdd(vpan.peers[peeridx]);
   } 
}
//======================================================
function iamTheCoordinator()
{
   return (vpan.isActive && vpan.coordinator == settings.vpan.my_url);
}
//======================================================
function coordResponseHandler(resp)
{
   if (!ownVpanStarted)
   {
      var data = '';
       
      // A chunk of data has been recieved.
      resp.on('data', function(chunk)
      {
         data += chunk;
      });
       
      // The whole response has been received. Print out the result.
      resp.on('end', function()
      {
         if (!ownVpanStarted)
         {
            try
            {
               var peerVpan = JSON.parse(data);
               
               for (var i = 0; i < peerVpan.peers.length; i++)
               {
                  peerListAdd(peerVpan.peers[i]);
               }
               
               for (var i = 0; i < peerVpan.departed_peers.length; i++)
               {
                  departedPeerListAdd(peerVpan.departed_peers[i]);
               }
               
               for (var i = 0; i < peerVpan.frontEndServerList.length; i++)
               {
                  frontEndServerListAdd(peerVpan.frontEndServerList[i]);
               }
               
               for (var i = 0; i < peerVpan.nodes.length; i++)
               {
                  nodeListAdd(peerVpan.nodes[i]);
               }
               
               clearTimeout(vpanStartTimer);
               coordinatorKeepAliveCountDown = COORD_KA_COUNTDOWN;
               initCode1();
            }
            catch (e)
            {
               searchVpnPeer(1000); 
            }
         }
      });
   }
}
//======================================================
function updateVpanFromCoordinator()
{
   if (!ownVpanStarted)
   {
      if (vpan.isActive && vpan.coordinator && !iamTheCoordinator())
      {
         var urlStr = 'http://'+vpan.coordinator+':'+httpPort+'/vpan?id='+vpan.vpanId+'&pwd='+settings.vpan.password;
         console.log('updateVpanFromCoordinator():'+urlStr);
         http.get(urlStr, coordResponseHandler).on("error", function(err)
         {       
            searchVpnPeer(1000);  
         });  
      }
   }
}
//======================================================
function informNewPeer(dest_addr, new_peer_addr, new_peer_node_address_base, errorCount, callBack)
{
   console.log("Inform new peer:" + new_peer_addr + ":" + new_peer_node_address_base);
   var daddr = dest_addr;
   var naddr = new_peer_addr;
   var npbase = new_peer_node_address_base;
   var ec = errorCount;
   var transacActive = true;
   var transacTimer = setTimeout(function()
   {
      transacActive = false;
      vpan.peers[gpg_arg.peeridx].group = [];
      if (callBack)
      {
         if (iamTheCoordinator())
         {
            vapanPeerDepartedHandler(daddr);
         }
         else
         {
            searchVpnPeer(60000); 
         }
         callBack();
      }
   }, VPAN_TRANSAC_TIMEOUT);  
   const options = 
   {
      hostname: dest_addr,
      port: httpPort,
      path: '/newpeer?ip_address='+new_peer_addr+'&node_address_base='+new_peer_node_address_base+'&pwd='+settings.vpan.password+'&vpanid='+settings.vpan.id,
      method: 'PUT',
   };
   
   //console.log("PUT to:"+options.hostname+':'+options.port);

   const req = http.request(options, function(res)
   {
      //console.log(res.statusCode);
      res.setEncoding('utf8');
      res.on('data', function(chunk)
      {
         //console.log('Chunk:'+chunk);
      });
      
      res.on('end', function()
      {
         if (transacActive)
         {
            clearTimeout(transacTimer);
            if (callBack)
            {
               callBack();
            }
         }
      });
   });

   req.on('error', function(e) 
   {
      if (transacActive)
      {
         clearTimeout(transacTimer);
         ec++;
         console.error(`problem with request (D): ${e.message}:`+ec);
   
         if (ec <= MAX_PEER_EXCHANGE_ERROR_COUNT)
         {
            informNewPeer(daddr, naddr, npbase, ec, callBack); //retry
         }
         else
         {
            if (iamTheCoordinator())
            {
               vapanPeerDepartedHandler(daddr);
            }
            else
            {
               searchVpnPeer(60000); 
            }
         }
      }
   });
   req.end();
}
//======================================================
function informJoin()
{     
   informNewPeer(vpan.coordinator, settings.vpan.my_url, vpan.node_address_base, 0, updateVpanFromCoordinator);
}
//======================================================
function nodeAddressBaseResponseHandler(resp)
{
   if (!ownVpanStarted)
   {
      var data = '';
       
      // A chunk of data has been recieved.
      resp.on('data', function(chunk)
      {
         data += chunk;
         //console.log("chunk:"+data);
      });
       
      // The whole response has been received. Print out the result.
      resp.on('end', function()
      {
         if (!ownVpanStarted)
         {
            try
            {
               var obj = JSON.parse(data);
               if (obj && obj.node_address_base != null)
               {
                  vpan.node_address_base = +obj.node_address_base;
                  gw_id = vpan.node_address_base + 1;
                  var newNode = {
                        eui64:"gateway", 
                        panAddr:gw_id, 
                        gw:vpan.node_address_base, 
                        vpanAddr:gw_id
                     };
                  nodeListAdd(newNode);
                  //console.log("vpan:"+JSON.stringify(vpan));
               }
               
               if (vpan.node_address_base < 1000000)
               {
                  informJoin();
               }
               else
               {
                  searchVpnPeer(5000);    
               }
            }
            catch (e)
            {
               searchVpnPeer(1000); 
            } 
         }
      });
   }
}
//======================================================
function get_node_address_base()
{
   var urlStr = 'http://'+vpan.coordinator+':'+httpPort+'/node_address_base?ip_address='+settings.vpan.my_url+'&pwd='+settings.vpan.password;
   console.log('get_node_address_base():'+urlStr);
   http.get(urlStr, nodeAddressBaseResponseHandler).on("error", function(err)
   {
      console.log("Error: " + err.message);
      //Can not join the VPAN
      searchVpnPeer(5000); 
   });
}
//======================================================
function peerResponseHandler(resp)
{
   if (!ownVpanStarted)
   {
      var data = '';
       
      // A chunk of data has been received.
      resp.on('data', function(chunk)
      {
         data += chunk;
      });
       
      // The whole response has been received. Print out the result.
      resp.on('end', function()
      {
         if (!ownVpanStarted)
         {
            try
            {
               var peerVpan = JSON.parse(data);
               if (peerVpan.isActive && peerVpan.coordinator && 
                        peerVpan.node_address_base >= 0 &&
                        peerVpan.nodes && peerVpan.peers &&
                        peerVpan.vpanId == settings.vpan.id)
               {
                  if (peerVpan.coordinator != settings.vpan.my_url)
                  {
                     //We are done if an active peer is found: 
                     vpan = peerVpan;
                     
                     peerListAdd({
                        'ip_address':settings.vpan.peers[resp.cng_arg.peeridx].ip_address,
                        'node_address_base':peerVpan.node_address_base
                     });
                           
                     peerListRemove(settings.vpan.my_url);
                     departedPeerListRemove(settings.vpan.my_url);
              
                     console.log("Join VPAN:"+JSON.stringify(vpan));
                     get_node_address_base();
                  }
                  else
                  {
                     searchVpnPeer(60000);
                  }
               }
               else
               {
                  //Keep on searching for a active peer:
                  contactNextPeer(resp.cng_arg);
               }
            }
            catch (e)
            {
               contactNextPeer(resp.cng_arg);
            } 
         }
      });
   }
}
//======================================================
function contactNextPeer(cnp_arg)
{
   if (!ownVpanStarted)
   {
      cnp_arg.peeridx++;
      if (cnp_arg.peeridx < settings.vpan.peers.length)
      {
         var urlStr = 'http://'+settings.vpan.peers[cnp_arg.peeridx].ip_address+':'+httpPort+'/vpan?id='+settings.vpan.id+'&pwd='+settings.vpan.password;
         console.log('contactNextPeer():'+urlStr);
         http.get(urlStr, function (resp)
         {
            resp.cng_arg = cnp_arg; 
            peerResponseHandler(resp);
         }).on("error", function(err)
         {
            //console.log("Error: " + err.message);
            contactNextPeer(cnp_arg);
         });
      }
      else
      {
         clearTimeout(vpanStartTimer);
         startOwnVpan();
      }
   }
}
//======================================================
function searchVpnPeer(delay)
{
   if (!ownVpanStarted)
   {
      vpan.isActive = false;
      vpan.vpanId = 0;
      vpan.coordinator = '';
      vpan.peers = [];
      vpan.departed_peers = [];
      vpan.frontEndServerList = [];
      vpan.node_address_base = 0;
      vpan.nodes = [];
      
      var cnp_arg = {'peeridx':-1};
      var randDelay = delay + getRandomInt(1, 2000);
      setTimeout(function()
      {
         contactNextPeer(cnp_arg);
      }, randDelay); 
   }
}
//======================================================
function startOwnVpan()
{
   if (!vpan.isActive)
   {
      ownVpanStarted = true;
      vpan.isActive = true;
      vpan.vpanId = settings.vpan.id;
      vpan.coordinator = settings.vpan.my_url;
      console.log("Start VPAN:"+vpan.vpanId);
      initCode1();
   }
}
//======================================================
function contact_vpan_peers()
{
   //Set the http server command handlers:
   http_get('/vpan', appFrontGetVpanHandler);
   http_get('/node_address_base', appFrontGetNodeAddressBaseHandler); 
   http_get('/peer_node_count', appFrontGetPeerNodeCountHandler);   
   http_get('/peer_group', appFrontGetPeerGroupHandler);
   http_get('/peer_nodes', appFrontGetPeerNodesHandler);
   http_get('/peer_nodes/:id', appFrontGetPeerNodesIdHandler);
   http_get('/peer_nwk/:nui/node', appFrontGetPeerNwkNuiNodeHandler);
   http_put('/newpeer', appFrontPutNewPeerHandler); 
   http_put('/frontendconnected', appFrontPutFrontEndConnectedHandler);
   http_put('/frontenddisconnected', appFrontPutFrontEndDisconnectedHandler);
   http_put('/frontenddata', appFrontPutFrontEndDataHandler);
   http_put('/coordinator', appFrontPutCoordinatorHandler);
   http_put('/nodejoin', appFrontPutNodeJoinHandler);  
   http_put('/iamstillhere', appFrontPutIamStillHereHandler);

   if (settings.vpan && settings.vpan.my_url && settings.vpan.id &&
         settings.vpan.peers && settings.vpan.peers.length >= 0)
   {
      vpanStartTimer = setTimeout(function()
      {
         startOwnVpan();
      }, VPAN_START_TIMEOUT);
      searchVpnPeer(0);
   }
   else
   {
      initCode1();
   }
}
//================================================================================
//Web front end command handlers
//================================================================================
//processes GET to /vpan
function appFrontGetVpanHandler(req, res)
{
   res.type('application/json');
   var tvpan = {};
   var id = req.param('id');
   var pwd = req.param('pwd');
   if (req && settings.vpan && id == settings.vpan.id && pwd == settings.vpan.password)
   {
      tvpan.isActive = vpan.isActive;
      tvpan.vpanId = vpan.vpanId;
      tvpan.coordinator = vpan.coordinator;
      tvpan.node_address_base = vpan.node_address_base;
      tvpan.peers = [];
      tvpan.departed_peers = [];
      tvpan.frontEndServerList = [];
      tvpan.nodes = [];
      
      for (var i = 0; i < vpan.peers.length; i++)
      {
         tvpan.peers.push({'ip_address': vpan.peers[i].ip_address, 'node_address_base': vpan.peers[i].node_address_base});
      }
      
      for (var i = 0; i < vpan.departed_peers.length; i++)
      {
         tvpan.departed_peers.push({'ip_address': vpan.departed_peers[i].ip_address, 'node_address_base': vpan.departed_peers[i].node_address_base});
      }
      
      for (var i = 0; i < vpan.frontEndServerList.length; i++)
      {
         tvpan.frontEndServerList.push(vpan.frontEndServerList[i]);
      }
      
      for (var i = 0; i < vpan.nodes.length; i++)
      {
         if (vpan.nodes[i].panAddr != gw_id)
         {
            tvpan.nodes.push(vpan.nodes[i]);
         }
      }
   }
   return res.json(tvpan);
}
//================================================================================
//processes GET to /node_address_base
function appFrontGetNodeAddressBaseHandler(req, res)
{
   res.type('application/json');
   var ip_address = req.param('ip_address');
   var pwd = req.param('pwd');
   var cnab = 1000000;
   
   if (pwd == settings.vpan.password && iamTheCoordinator())
   {
      var idx = peerListSearch(ip_address); 
      if (idx >= 0)
      {
         cnab = vpan.peers[idx].node_address_base;
      }
      else
      {
         idx = departedPeerListSearch(ip_address);
         if (idx >= 0)
         {
            cnab = vpan.departed_peers[idx].node_address_base;
         }
         else
         {         
            var avail_addr_base = -node_address_span;

            while (true)
            {   
               avail_addr_base += node_address_span;
               if (avail_addr_base == vpan.node_address_base)
               {
                  continue;
               }
               
               var found = false;
               for (var i = 0; i < vpan.peers.length; i++)
               {
                  if (avail_addr_base == vpan.peers[i].node_address_base)
                  {
                     found = true;
                     break;
                  }
               }
               
               if (found)
               {
                  continue;
               }
               
               for (var i = 0; i < vpan.departed_peers.length; i++)
               {
                  if (avail_addr_base == vpan.departed_peers[i].node_address_base)
                  {
                     found = true;
                     break;
                  }
               }
               
               if (!found)
               {
                  break;
               }
            }
               
            cnab = avail_addr_base;
         }
      }
   }
   //console.log("Assign node base:"+cnab);
   return res.json({"node_address_base": cnab});
}
//================================================================================
//processes PUT to /newpeer
function appFrontPutNewPeerHandler(req, res)
{
   res.type('application/json');
   if (vpan.isActive)
   {
      var ip_address = req.param('ip_address');
      var in_node_address_base = +req.param('node_address_base');
      var pwd = req.param('pwd');
      var vpanid = req.param('vpanid');
      
      if (vpanid == settings.vpan.id && pwd == settings.vpan.password && 
            ip_address && in_node_address_base != null)
      {
         peerListAdd({
            'ip_address':ip_address,
            'node_address_base':in_node_address_base
         });
                       
         if (iamTheCoordinator())
         {
            for (var i = 0; i < vpan.peers.length; i++)
            {
               if (vpan.peers[i].ip_address != ip_address)
               {
                  informNewPeer(vpan.peers[i].ip_address, ip_address, in_node_address_base, 0);
               }
            }
         }
      }
      return res.json({'response':'OK'});
   }
   else
   {
      return res.json({'response':'not in vpan'});
   }
}
//================================================================================
//processes PUT to /frontendconnected
function appFrontPutFrontEndConnectedHandler(req, res)
{
   //console.log('appFrontPutFrontEndConnectedHandler() enter');
   res.type('application/json');
   if (vpan.isActive)
   {
      departedPeerComeBackCheck(ipv6AddrToipv4Addr(req.connection.remoteAddress));
      if (peerListSearch(ipv6AddrToipv4Addr(req.connection.remoteAddress)) >= 0)
      {
         var ip_address = req.param('ip_address');
         if (peerListSearch(ip_address) >= 0)
         {
            frontEndServerListAdd({'ip_address':ip_address});
         }
      }
   }
   //console.log('frontEndServerList:'+JSON.stringify(vpan.frontEndServerList));
   return res.json({'response':'OK'});
}
//================================================================================
//processes PUT to /frontenddisconnected
function appFrontPutFrontEndDisconnectedHandler(req, res)
{
   //console.log('appFrontPutFrontEndDisconnectedHandler() enter');
   res.type('application/json');
   var ip_address = req.param('ip_address');
   if (vpan.isActive)
   {
      departedPeerComeBackCheck(ipv6AddrToipv4Addr(req.connection.remoteAddress));
      if (peerListSearch(ipv6AddrToipv4Addr(req.connection.remoteAddress)) >= 0)
      {
         if (peerListSearch(ip_address) >= 0)
         {
            frontEndServerListRemove(ip_address);
         }
      }
   }
   //console.log('frontEndServerList:'+JSON.stringify(vpan.frontEndServerList));
   return res.json({'response':'OK'});
}
//================================================================================
//processes PUT to /frontenddata
function appFrontPutFrontEndDataHandler(req, res)
{
   res.type('application/json');
   if (vpan.isActive)
   {
      departedPeerComeBackCheck(ipv6AddrToipv4Addr(req.connection.remoteAddress));
      if (peerListSearch(ipv6AddrToipv4Addr(req.connection.remoteAddress)) >= 0)
      {
         var fdata = req.param('fdata');
         //console.log('front end data:'+fdata);
         web_broadcast(fdata);
      }
   }
   return res.json({'response':'OK'});
}
//================================================================================
//processes PUT to /coordinator
function appFrontPutCoordinatorHandler(req, res)
{
   //console.log('appFrontPutCoordinatorHandler() enter');
   res.type('application/json');
   
   if (vpan.isActive)
   {
      departedPeerComeBackCheck(ipv6AddrToipv4Addr(req.connection.remoteAddress));
      if (peerListSearch(ipv6AddrToipv4Addr(req.connection.remoteAddress)) >= 0)
      {
   	   var newCordinatorIpaddr = req.param('ip_address');   
   	   if (newCordinatorIpaddr)
         {
   	      var idx = peerListSearch(newCordinatorIpaddr);
   	      if (idx >= 0)
   	      {
   	         vpan.coordinator = newCordinatorIpaddr;
   	         console.log("Coordinator: "+vpan.coordinator);
   	         coordinatorKeepAliveCountDown = COORD_KA_COUNTDOWN;
   	      }
         }
      }
   }
   
   return res.json({'response':'OK'});
}
//================================================================================
//processes PUT to /nodejoin
function appFrontPutNodeJoinHandler(req, res)
{
   res.type('application/json');
    
   if (vpan.isActive)
   {
      departedPeerComeBackCheck(ipv6AddrToipv4Addr(req.connection.remoteAddress));
      if (peerListSearch(ipv6AddrToipv4Addr(req.connection.remoteAddress)) >= 0)
      {
         var ip_address = req.param('ip_address');
         try
         {
            var new_node = JSON.parse(req.param('new_node'));
            if (ip_address && new_node)
            {
               var i = nodeListSearch(new_node);
               if (i >= 0 && vpan.nodes[i].gw == vpan.node_address_base)
               {
                  node_joined_another_pan_handler(vpan.nodes[i].panAddr);
               }
         
               nodeListAdd(new_node);
               
               //console.log('appFrontPutNodeJoinHandler():'+JSON.stringify(vpan.nodes));
                                  
               if (iamTheCoordinator())
               {
                  for (var i = 0; i < vpan.peers.length; i++)
                  {
                     if (vpan.peers[i].ip_address != ip_address)
                     {
                        vpanInformPeerNodeJoin(vpan.peers[i].ip_address, ip_address, new_node, 0);
                     }
                  }
               }
            }
         }
         catch (e)
         {
            return res.json({'response':'ERROR'});
         }

      }

      return res.json({'response':'OK'});
   }
   else
   {
      return res.json({'response':'not in vpan'});
   }
}
//======================================================
function vpanInformNodeJoin(newNode)
{
   //console.log('vpanInformNodeJoin() enter');
   if (!iamTheCoordinator())
   {
      vpanInformPeerNodeJoin(vpan.coordinator, settings.vpan.my_url, newNode, 0);
   }
   else
   {
      for (var i = 0; i < vpan.peers.length; i++)
      {
         vpanInformPeerNodeJoin(vpan.peers[i].ip_address, settings.vpan.my_url, newNode, 0);
      }
   }
}
//======================================================
function vpanInformPeerNodeJoin(dest_addr, ip_addr, newNode, errorCount, callBack)
{
   var daddr = dest_addr;
   var ipaddr = ip_addr;
   var nn = newNode;
   var ec = errorCount;
   var transacActive = true;
   var transacTimer = setTimeout(function()
   {
      transacActive = false;
      vapanPeerDepartedHandler(daddr);
      if (callBack)
      {
         callBack();
      }
   }, VPAN_TRANSAC_TIMEOUT);   
   
   const options = 
   {
      hostname: dest_addr,
      port: httpPort,
      path: '/nodejoin?ip_address='+ip_addr+'&new_node='+JSON.stringify(newNode),
      method: 'PUT',
   };
   
   //console.log("PUT to:"+options.hostname+':'+options.port);

   const req = http.request(options, function(res)
   {
      //console.log(res.statusCode);
      res.setEncoding('utf8');
      res.on('data', function(chunk)
      {
         //console.log('Chunk:'+chunk);
      });
      
      res.on('end', function()
      {
         if (transacActive)
         {
            clearTimeout(transacTimer);
            if (callBack)
            {
               callBack();
            }
         }
      });
   });

   req.on('error', function(e) 
   {
      if (transacActive)
      {
         clearTimeout(transacTimer);
         ec++;
         console.error(`problem with request (B): ${e.message}:`+ec);
         if (ec <= MAX_PEER_EXCHANGE_ERROR_COUNT)
         {
            vpanInformPeerNodeJoin(daddr, ipaddr, nn, ec, callBack); //retry
         }
         else
         {
            vapanPeerDepartedHandler(daddr);
            if (callBack)
            {
               callBack();
            }
         }
      }
   });
   req.end();
}
//================================================================================
//processes PUT to /iamstillhere
function appFrontPutIamStillHereHandler(req, res)
{
   res.type('application/json');
    
   if (vpan.isActive)
   {
      var ip_address = ipv6AddrToipv4Addr(req.connection.remoteAddress);
      departedPeerComeBackCheck(ip_address);
      console.log("Peer still there:" + ip_address);
      return res.json({'response':'OK'});
   }
   else
   {
      return res.json({'response':'not in vpan'});
   }
}
//=======================================================
//get ALL authorized nodes for network <nui>
//processes GET to /nwk/<nui>/node
function getPeerNwkNuiNode(peer_arg, callBack, arg)  
{  
   var transacActive = true;
   var transacTimer = setTimeout(function()
   {
      transacActive = false;
      vpan.peers[peer_arg.peeridx].NwkNuiNode = [];
      var peeridx = peer_arg.peeridx;
      vapanPeerDepartedHandler(peer_arg.dest_addr);
      if (callBack)
      {
         peer_arg.peeridx = peeridx - 1;
         callBack(peer_arg, arg);
      }
   }, VPAN_TRANSAC_TIMEOUT); 
   
   const options = 
   {
      hostname: peer_arg.dest_addr,
      port: httpPort,
      path: '/peer_nwk/'+peer_arg.nui+'/node',
      method: 'GET',
   };
   
   //console.log("GET from:"+options.hostname+':'+options.port);

   const req = http.request(options, function(res)
   {
      var data = '';
      
      //console.log(res.statusCode);
      res.setEncoding('utf8');
      res.on('data', function(chunk)
      {
         data += chunk;
         //console.log('Chunk:'+chunk);
      });
      
      res.on('end', function()
      {
         if (transacActive)
         {
            clearTimeout(transacTimer);
            try
            {
               vpan.peers[peer_arg.peeridx].NwkNuiNode = JSON.parse(data);
               //console.log('getPeerNwkNuiNode():'+'['+peer_arg.peeridx+']='+JSON.stringify(vpan.peers[peer_arg.peeridx].NwkNuiNode));
               if (callBack)
               {
                  callBack(peer_arg, arg);
               }
            }
            catch (e)
            {
               console.error(`problem with request (11a): ${e.message}`);
               vpan.peers[peer_arg.peeridx].NwkNuiNode = [];
               var peeridx = peer_arg.peeridx;
               vapanPeerDepartedHandler(peer_arg.dest_addr);
               if (callBack)
               {
                  peer_arg.peeridx = peeridx - 1;
                  callBack(peer_arg, arg);
               }
            }
         }
      });
   });

   req.on('error', function(e) 
   {
      if (transacActive)
      {
         clearTimeout(transacTimer);
         console.error(`problem with request (11): ${e.message}`);
         vpan.peers[peer_arg.peeridx].NwkNuiNode = [];
         var peeridx = peer_arg.peeridx;
         vapanPeerDepartedHandler(peer_arg.dest_addr);
         if (callBack)
         {
            peer_arg.peeridx = peeridx - 1;
            callBack(peer_arg, arg);
         }
      }
   });
   req.end();
}
//================================================================================
function getNextPeerNwkNuiNode(peer_arg, callback)
{
   if (!vpan.isActive)
   {
      if (callback)
      {
         callback(null,[]);
      }
   }
   else
   {
      peer_arg.peeridx++;
      if (peer_arg.peeridx < vpan.peers.length)
      {
         peer_arg.dest_addr = vpan.peers[peer_arg.peeridx].ip_address;
         getPeerNwkNuiNode(peer_arg, getNextPeerNwkNuiNode, callback);
      }
      else
      {
         var items = [];
         for (var i = 0; i < vpan.peers.length; i++)
         {
            items = items.concat(vpan.peers[i].NwkNuiNode);
         }
         
         if (callback)
         {
            callback(null, items)
         }
         //console.log('getNextPeerNwkNuiNode():'+JSON.stringify(items));   
      }
   }
}
//================================================================================
function appFrontGetPeerNwkNuiNodeHandler(req, res)
{
   //console.log('appFrontGetPeerNwkNuiNodeHandler() enter');
   var nui = req.params.nui;

   //query network <nui> collection
   departedPeerComeBackCheck(ipv6AddrToipv4Addr(req.connection.remoteAddress));
   db_find_nwk_nui_nodes(req.params.nui, function(err, items)
   {
      if (!err)
      {
         //respond with JSON data
         res.statusCode = 200;
         res.type('application/json');
         res.json(items);
         console.log("Sent all connected nodes to " + req.connection.remoteAddress);
      }
      else
      {
         logError(err);
         res.statusCode = 400;
         return res.send('Error 400: get unsuccessful');
      }
   });        
}
//==================================================
//get group list
//processes GET to /group

function getPeerGroup(gpg_arg, callBack, arg)  
{  
   var transacActive = true;
   var transacTimer = setTimeout(function()
   {
      transacActive = false;
      vpan.peers[gpg_arg.peeridx].group = [];
      var peeridx = gpg_arg.peeridx;
      vapanPeerDepartedHandler(gpg_arg.dest_addr);
      if (callBack)
      {
         gpg_arg.peeridx = peeridx - 1;
         callBack(gpg_arg, arg);
      }
   }, VPAN_TRANSAC_TIMEOUT);
   
   const options = 
   {
      hostname: gpg_arg.dest_addr,
      port: httpPort,
      path: '/peer_group',
      method: 'GET',
   };
   
   //console.log("GET from:"+options.hostname+':'+options.port);

   const req = http.request(options, function(res)
   {
      var data = '';
      
      //console.log(res.statusCode);
      res.setEncoding('utf8');
      res.on('data', function(chunk)
      {
         data += chunk;
         //console.log('Chunk:'+chunk);
      });
      
      res.on('end', function()
      {
         if (transacActive)
         {
            clearTimeout(transacTimer);
            try
            {
               vpan.peers[gpg_arg.peeridx].group = JSON.parse(data);
               //console.log('getPeerGroup():'+'['+gpg_arg.peeridx+']='+JSON.stringify(vpan.peers[gpg_arg.peeridx].group));
               if (callBack)
               {
                  callBack(gpg_arg, arg);
               }
            }
            catch (e)
            {
               console.error(`problem with request (12a): ${e.message}`);
               vpan.peers[gpg_arg.peeridx].group = [];
               var peeridx = gpg_arg.peeridx;
               vapanPeerDepartedHandler(gpg_arg.dest_addr);
               if (callBack)
               {
                  gpg_arg.peeridx = peeridx - 1;
                  callBack(gpg_arg, arg);
               }
            }
         }
      });
   });

   req.on('error', function(e) 
   {
      if (transacActive)
      {
         clearTimeout(transacTimer);
         console.error(`problem with request (12): ${e.message}`);
         vpan.peers[gpg_arg.peeridx].group = [];
         var peeridx = gpg_arg.peeridx;
         vapanPeerDepartedHandler(gpg_arg.dest_addr);
         if (callBack)
         {
            gpg_arg.peeridx = peeridx - 1;
            callBack(gpg_arg, arg);
         }
      }
   });
   req.end();
}
//================================================================================
function getNextPeerGroup(gpg_arg, callback)
{
   if (!vpan.isActive)
   {
      if (callback)
      {
         callback(null,[]);
      }
   }
   else
   {
      gpg_arg.peeridx++;
      if (gpg_arg.peeridx < vpan.peers.length)
      {
         gpg_arg.dest_addr = vpan.peers[gpg_arg.peeridx].ip_address;
         getPeerGroup(gpg_arg, getNextPeerGroup, callback);
      }
      else
      {
         //console.log('getNextPeerGroup()');
         //query network <nui> collection
         var items = [];
         for (var i = 0; i < vpan.peers.length; i++)
         {
            items = items.concat(vpan.peers[i].group);
         }
         
         if (callback)
         {
            callback(null, items);
         }
      }
   }
}
//================================================================================
function appFrontGetPeerGroupHandler(req, res)
{
   //console.log('appFrontGetPeerGroupHandler() enter');
   if (!vpan.isActive)
   {
      res.statusCode = 200;
      res.type('application/json');
      res.json([]);
   }
   else
   {      
      departedPeerComeBackCheck(ipv6AddrToipv4Addr(req.connection.remoteAddress));
   //query network <nui> collection
      db_find_groups(null, function(err, items)
      {
         if (!err)
         {
            //respond with JSON data
            res.statusCode = 200;
            res.type('application/json');
            res.json(items);
            console.log("Sent groups");
         }
         else
         {
            logError(err);
            res.statusCode = 400;
            return res.send('Error 400: get groups unsuccessful');
         }
      });
   }
}
//============================
function getPeerNodeCount(gnpnc_arg, callBack, arg)  
{  
   var transacActive = true;
   var transacTimer = setTimeout(function()
   {
      transacActive = false;
      vpan.peers[gnpnc_arg.peeridx].nodeCount = 0;
      var peeridx = gnpnc_arg.peeridx;
      vapanPeerDepartedHandler(gnpnc_arg.dest_addr);
      if (callBack)
      {
         gnpnc_arg.peeridx = peeridx - 1;
         callBack(gnpnc_arg, arg);
      }
   }, VPAN_TRANSAC_TIMEOUT);
      
   const options = 
   {
      hostname: gnpnc_arg.dest_addr,
      port: httpPort,
      path: '/peer_node_count',
      method: 'GET',
   };
   
   //console.log("GET from:"+options.hostname+':'+options.port);

   const req = http.request(options, function(res)
   {
      var data = '';
      
      //console.log(res.statusCode);
      res.setEncoding('utf8');
      res.on('data', function(chunk)
      {
         data += chunk;
         //console.log('Chunk:'+chunk);
      });
      
      res.on('end', function()
      {
         if (transacActive)
         {
            clearTimeout(transacTimer);
            try
            {
               vpan.peers[gnpnc_arg.peeridx].nodeCount = +JSON.parse(data);
               //console.log('getPeerNodeCount():'+'['+gnpnc_arg.peeridx+']='+vpan.peers[gnpnc_arg.peeridx].nodeCount);
               if (callBack)
               {
                  callBack(gnpnc_arg, arg);
               }
            }
            catch (e)
            {
               console.error(`problem with request (13a): ${e.message}`);
               vpan.peers[gnpnc_arg.peeridx].nodeCount = 0;
               var peeridx = gnpnc_arg.peeridx;
               vapanPeerDepartedHandler(gnpnc_arg.dest_addr);
               if (callBack)
               {
                  gnpnc_arg.peeridx = peeridx - 1;
                  callBack(gnpnc_arg, arg);
               }
            }
         }
      });
   });

   req.on('error', function(e) 
   {
      if (transacActive)
      {
         clearTimeout(transacTimer);
         console.error(`problem with request (13): ${e.message}`);
         vpan.peers[gnpnc_arg.peeridx].nodeCount = 0;
         var peeridx = gnpnc_arg.peeridx;
         vapanPeerDepartedHandler(gnpnc_arg.dest_addr);
         if (callBack)
         {
            gnpnc_arg.peeridx = peeridx - 1;
            callBack(gnpnc_arg, arg);
         }
      }
   });
   req.end();
}
//================================================================================
function getNextPeerNodeCount(gnpnc_arg, callback)
{
   if (!vpan.isActive)
   {
      if (callback)
      {
         callback(null,0);
      }
   }
   else
   {
      gnpnc_arg.peeridx++;
      if (gnpnc_arg.peeridx < vpan.peers.length)
      {
         gnpnc_arg.dest_addr = vpan.peers[gnpnc_arg.peeridx].ip_address;
         getPeerNodeCount(gnpnc_arg, getNextPeerNodeCount, callback);
      }
      else
      {
         var count = 0;
         for (var i = 0; i < vpan.peers.length; i++)
         {
            count += vpan.peers[i].nodeCount;
         }
         
         if (callback)
         {
            callback(null, count);
         }
         
         //console.log('getNextPeerNodeCount():'+count);
      }
   }
}
//================================================================================
function appFrontGetPeerNodeCountHandler(req, res)
{  
   //console.log('appFrontGetPeerNodeCountHandler() enter');
   
   if (!vpan.isActive)
   {
      res.statusCode = 200;
      res.send("0");
   }
   else
   {
      departedPeerComeBackCheck(ipv6AddrToipv4Addr(req.connection.remoteAddress));
      db_node_count({ lifetime: { $gt: 0 } }, function(err, count)
      {
         if (err)
         {
            res.statusCode = 400;
            res.type('application/json');
            res.send("400");
         }
         else
         {
            res.statusCode = 200;
            res.send(count.toString());
         }
      });
   }
}
//================================================================================
function appFrontGetPeerNodesIdHandler(req, res)
{
   //console.log('appFrontGetPeerNodesIdHandler() enter');
   if (!vpan.isActive)
   {
      res.statusCode = 400;
      res.send("400");
   }
   else
   { 
      var id = +req.params.id;
      id = vpanAddrTopanAddr(id);
      departedPeerComeBackCheck(ipv6AddrToipv4Addr(req.connection.remoteAddress));   
      
      db_node_find({ _id: id }, function(err, items)
      {
         if (err || items[0] == null)
         {
            res.statusCode = 400;
            res.send("400");
            if (err)
            {
               console.log(err);
            }
         }
         else
         {
            res.statusCode = 200;
            res.type('application/json');
            get_node_id_callback(items);
            vpan_adjust_params(items[0]);
            res.json(items[0]);
         }
      });
   }
}
//=========================================

function getPeerNodes(gpn_arg, callBack, arg)  
{  
   var transacActive = true;
   var transacTimer = setTimeout(function()
   {
      transacActive = false;
      vpan.peers[gpn_arg.peeridx].nodes = [];
      var peeridx = gpn_arg.peeridx;
      vapanPeerDepartedHandler(gpn_arg.dest_addr);
      if (callBack)
      {
         gpn_arg.peeridx = peeridx - 1;
         callBack(gpn_arg, arg);
      }
   }, VPAN_TRANSAC_TIMEOUT); 
   
   const options = 
   {
      hostname: gpn_arg.dest_addr,
      port: httpPort,
      path: '/peer_nodes',
      method: 'GET',
   };
   
   //console.log("GET from:"+options.hostname+':'+options.port);

   const req = http.request(options, function(res)
   {
      var data = '';
      
      //console.log(res.statusCode);
      res.setEncoding('utf8');
      res.on('data', function(chunk)
      {
         data += chunk;
         //console.log('Chunk:'+chunk);
      });
      
      res.on('end', function()
      {
         if (transacActive)
         {
            clearTimeout(transacTimer);
            try
            {
               vpan.peers[gpn_arg.peeridx].nodes = JSON.parse(data);
               //console.log('getPeerNodes():'+'['+gpn_arg.peeridx+']='+JSON.stringify(vpan.peers[gpn_arg.peeridx].nodes));
               if (callBack)
               {
                  callBack(gpn_arg, arg);
               }
            }
            catch (e)
            {
               console.error(`problem with request (10): ${e.message}`);
               vpan.peers[gpn_arg.peeridx].nodes = [];
               var peeridx = gpn_arg.peeridx;
               vapanPeerDepartedHandler(gpn_arg.dest_addr);
               if (callBack)
               {
                  gpn_arg.peeridx = peeridx - 1;
                  callBack(gpn_arg, arg);
               }
            }
         }
      });
   });

   req.on('error', function(e) 
   {
      if (transacActive)
      {
         clearTimeout(transacTimer);
         console.error(`problem with request (10): ${e.message}`);
         vpan.peers[gpn_arg.peeridx].nodes = [];
         var peeridx = gpn_arg.peeridx;
         vapanPeerDepartedHandler(gpn_arg.dest_addr);
         if (callBack)
         {
            gpn_arg.peeridx = peeridx - 1;
            callBack(gpn_arg, arg);
         }
      }
   });
   req.end();
}
//================================================================================
function getNextPeerNodes(gpn_arg, callback)
{
   if (!vpan.isActive)
   {
      if (callback)
      {
         callback(null,[]);
      }
   }
   else
   {
      gpn_arg.peeridx++;
      if (gpn_arg.peeridx < vpan.peers.length)
      {
         gpn_arg.dest_addr = vpan.peers[gpn_arg.peeridx].ip_address;
         getPeerNodes(gpn_arg, getNextPeerNodes, callback);
      }
      else
      {
         var items = [];
         for (var i = 0; i < vpan.peers.length; i++)
         {
            items = items.concat(vpan.peers[i].nodes);
         }      
         if (callback)
         {
            callback(null, items);
         }
      }
   }
}
//================================================================================
function appFrontGetPeerNodesHandler(req, res)
{
   //console.log('appFrontGetPeerNodesHandler() enter');
   
   if (!vpan.isActive)
   {
      res.statusCode = 200;
      res.type('application/json');
      res.json([]);
   }
   else
   { 
      departedPeerComeBackCheck(ipv6AddrToipv4Addr(req.connection.remoteAddress));
      db_node_find({}, function(err, items)
      {
         if (err)
         {
            res.statusCode = 400;
            res.send("400");
            console.log('appFrontGetPeerNodesHandler() error:'+err);
         }
         else
         {
            vpanNodesListPurge(items);
               
            res.statusCode = 200;
            res.type('application/json');

            get_nodes_callback(items);
            for (var i = 0; i < items.length; ++i)
            {                 
               vpan_adjust_params(items[i]);              
            }
            res.json(items);
         }
         //console.log('appFrontGetPeerNodesHandler() exit');
      });
   }
}
//================================================================================
function getNodeIdFromPeerGateway(id, gw, callback)
{            
   for (var i = 0; i < vpan.peers.length; i++)
   {
      //console.log("id:"+id+",node_address_base:"+vpan.peers[i].node_address_base+",x:"+(vpan.peers[i].node_address_base + node_address_span));
      if (gw == vpan.peers[i].node_address_base)
      {
         const options = 
         {
            hostname: vpan.peers[i].ip_address,
            port: httpPort,
            path: '/peer_nodes/'+id,
            method: 'GET',
         };
         
         //console.log("GET from:"+options.hostname+':'+options.port+':'+options.path);

         const req = http.request(options, function(res)
         {
            var data = '';
            
            //console.log(res.statusCode);
            res.setEncoding('utf8');
            res.on('data', function(chunk)
            {
               data += chunk;
               //console.log('Chunk:'+chunk);
            });
            
            res.on('end', function()
            {
               try
               {
                  item = JSON.parse(data);
                  if (callback)
                  {
                     callback(null, item);
                  }
               }
               catch (e)
               {
                  console.error(`problem with request (14): ${e.message}`);
                  if (callback)
                  {
                     callback(e, null);
                  }
               }
            });
         });

         req.on('error', function(e) 
         {
            console.error(`problem with request (14): ${e.message}`);
            if (callback)
            {
               callback(e, null);
            }
         });
         req.end();
                 
         break;
      }
   }
   
   if (i == vpan.peers.length)
   {
      if (callback)
      {
         callback('GW not found', null);
      }
   }    
}
//================================================================================
//multi-GW system hooks:
//================================================================================
function setup(hooks)
{  
   if (hooks)
   {
      web_broadcast = hooks.web_broadcast;
      http_get = hooks.http_get;
      http_put = hooks.http_put;
      settings = hooks.settings;       
      if (hooks.httpPort)
      {
         httpPort = hooks.httpPort;
      }
      
      db_find_nwk_nui_nodes = hooks.db_find_nwk_nui_nodes;
      db_find_groups = hooks.db_find_groups;
      db_node_count = hooks.db_node_count;
      db_node_find = hooks.db_node_find;
      get_node_id_callback = hooks.get_node_id_callback;
      get_nodes_callback = hooks.get_nodes_callback;
      init_callback = hooks.init_callback;
      node_joined_another_pan_handler = hooks.node_joined_another_pan_handler;
   }

   return this;
}
//================================================================================
//multi-GW module API:
//================================================================================
module.exports = 
{
   setup: setup,
   assignVpanAddress: assignVpanAddress,
   panAddrTovpanAddr: panAddrTovpanAddr,
   vpanAddrTopanAddr: vpanAddrTopanAddr,
   vpanAddrToGw: vpanAddrToGw,
   inform_vpan_peers_frontend_connected: inform_vpan_peers_frontend_connected,
   inform_vpan_peers_frontend_disconnected: inform_vpan_peers_frontend_disconnected,
   broadcastFrontEndDataToServers: broadcastFrontEndDataToServers,
   vpan_adjust_params: vpan_adjust_params,
   vpan_restore_params: vpan_restore_params,
   contact_vpan_peers: contact_vpan_peers,
   getNextPeerNwkNuiNode: getNextPeerNwkNuiNode,
   getNextPeerGroup: getNextPeerGroup,
   getNextPeerNodeCount: getNextPeerNodeCount,
   getNodeIdFromPeerGateway: getNodeIdFromPeerGateway,
   getNextPeerNodes: getNextPeerNodes,
   vpanNodesListPurge: vpanNodesListPurge,
   isGateway: isGateway,
   gw_id: gw_id,
   vpanIsActive: vpanIsActive,
   vpanId: vpanId,
   vpanNodeAddressBase: vpanNodeAddressBase
};
