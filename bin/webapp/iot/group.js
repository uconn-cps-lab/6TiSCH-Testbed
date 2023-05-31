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


var iot = iot || { };

iot.grid = {};
iot.grid.refreshing = false;
iot.grid.q = 0;
iot.grid.filter = "all";
iot.grid.nodes = new Array();
iot.grid.backup = new Array();

iot.nodes = new Array();
iot.networks = new Array();
iot.groups = new Array();

iot.selected_group = -1;

iot.num_nodes = 0;

iot.active = { };
iot.active.manage_targets = false;

/* -------------------------------------------------------------------------- */

iot.get_elapsed_time = function(timestamp) {
  var then = new Date(timestamp);
  var now = new Date();
  var buf = '';
  var elapsed = 0;
  var use_minutes = false, use_hours = false, use_days = false;

  if (then.getTime() > now.getTime())
    return 'get_time_elapsed(): warning: "earlier" timestamp showing future date';
  
  elapsed = (now.getTime() / 1000) - (then.getTime() / 1000);
  
  if (elapsed >= 60)
    use_minutes = true;
  if (elapsed >= 3600)
    use_hours = true;
  if (elapsed >= 86400)
    use_days = true;
  
  if (use_days)
    buf += '' + Math.floor((elapsed / 86400)) + 'd';
  if (use_hours)
    buf += (use_days ? ' ' : '') + Math.floor((elapsed % 86400) / 3600) + 'h';
  if (use_minutes)
    buf += (use_hours ? ' ' : '') + Math.floor(((elapsed % 86400) % 3600) / 60) + 'm';
  
  buf += (use_minutes ? ' ' : '') + Math.floor(elapsed % 60) + 's ago';
  
  return buf;
};

iot.get_node_index = function(nid) {
  for (var i = 0; i < iot.node_info.length; i++)
    if (iot.node_info[i]["id"] == nid)
      return i;

  return false;
};

iot.node_in_group = function(gid, nid) {
  var index = iot.get_node_index(nid);

  if (!index)
    return false;

  if (iot.node_info[index]["id"] == gid)
    return true;

  return false;
};

iot.url_parse_group = function() {
  var result = false;
  var temp = [];

  window.location.search.substr(1).split("&").forEach(function(item) {
    temp = item.split("=");

    if (temp[0] === "group")
      result = temp[1];
  });

  return result;
};

/* acquiring the list of individually unique gateways contained in the list
 * of a groups' node list will allow us to ONLY poll the server for the
 * networks we actually need.  this is especially important for high capacity
 * networks with multitudes of nodes.
 */
iot.get_unique_networks = function() {
  var group_index = iot.get_group_index_by_id(iot.selected_group);
  var gateways = [];
  
  for (var i = 0; i < iot.groups[group_index].nodes.length; i++) {
    var found = false;
    
    for (var j = 0; j < gateways.length; j++)
      if (iot.groups[group_index].nodes[i]["gateway"] == gateways[j])
        found = true;
    
    if (found)
      continue;
    
    gateways.push(iot.groups[group_index].nodes[i]["gateway"]);
  }
  
  return gateways;
};

iot.get_networks = function() {
  $.ajax({
    url: "/nwk",
    success: function(d) {
      iot.networks = d;
      
      /* when this project goes from alpha to beta and this "backup" test data
       * is no longer needed, this entire ajax needs to be removed and everything
       * inside its' success callback (minus backup=backup) put here inside the
       * first callback
       */
      $.ajax({
        url: "node/info.json",
        success: function(backup_data) {
          iot.grid.backup = backup_data;
          
          iot.update_num_connected();

          iot.init_dependant_binds();

          /* pushing a refresh event for the grid here, it's ensured that we have
           * already loaded the network (gateways) and have successfully
           * loaded in my bs dummy data
           */
          $("#grid").trigger("refreshStart");
        }
      });
    }
  });
};

iot.grid_header = function() {
  var buf = '';
  
  buf += '<td>Status</td>';
  buf += '<td>Node</td>';
  buf += '<td>Sensors</td>';
  buf += '<td>Power</td>';
  buf += '<td>Actuator</td>';

  return buf;
};

iot.get_status = function(type, value) {
  switch (type) {
    /* eventually overall status needs to be replaced with a fallthrough block
     * based upon the values gathered from the other sensors
     */
    case 'status':
	  if (value > 0) return "<div class='gridicon status_ok'></div>";
	  else return "<div class='gridicon status_inactive'></div>";
    case 'temperature':
	  if (value > 0) return "<div class='gridicon temp_ok'></div>";
	  else return "<div class='gridicon temp_inactive'></div>";
    case 'luminosity':
	  if (value > 0) return "<div class='gridicon lum_ok'></div>";
	  else return "<div class='gridicon lum_inactive'></div>";
    case 'sound':
	  if (value > 0) return "<div class='gridicon sound_ok'></div>";
	  else return "<div class='gridicon sound_inactive'></div>";
    case 'humidity':
	  if (value > 0) return "<div class='gridicon hum_ok'></div>";
	  else return "<div class='gridicon hum_inactive'></div>";
    case 'motion':
	  if (value > 0) return "<div class='gridicon mot_ok'></div>";
	  else return "<div class='gridicon mot_inactive'></div>";
    case 'actuator_light':
	  if (value > 0) return "<div class='gridicon act_on'></div>";
	  else return "<div class='gridicon act_off'></div>";
    case 'actuator_gpio':
	  if (value > 0) return "<div class='gridicon act_on'></div>";
	  else return "<div class='gridicon act_off'></div>";
    default:
      break;
  }
};

iot.get_network_index_by_gateway = function(gateway) {
  for (var i = 0; i < iot.networks.length; i++)
    if (iot.networks[i]["_id"] == gateway)
      return i;
};

iot.get_group_index_by_id = function(id) {
  for (var i = 0; i < iot.groups.length; i++)
    if (iot.groups[i]["id"] == id)
      return i;
};

/* this is not the same populate_nodes() used by list.x */
iot.populate_nodes = function() {
  /* remove all nodes from the node list so we don't have strays in our new grid render
   * (popping preserves references, but we don't have any here, so we do it the faster way)
   */
  iot.grid.nodes = [];
//  var group_index = iot.get_group_index_by_id(iot.selected_group);
  
//  while (iot.grid.nodes.length)
//    iot.grid.nodes.pop();

//  for (var i = 0; i < iot.groups[group_index].nodes.length; i++) {
//    var gateway = iot.groups[group_index].nodes[i]["gateway"];
//    var network_index = iot.get_network_index_by_gateway(gateway);
//    
//    if (!iot.networks[network_index].nodes)
//      continue;
//    
//    for (var j = 0; j < iot.networks[network_index].nodes.length; j++)
//      if (iot.groups[group_index].nodes[i]["node"] == iot.networks[network_index].nodes[j]["_id"])
//        iot.grid.nodes.push(iot.networks[network_index].nodes[j]);
//  }

  var group = iot.url_parse_group();
  
  $("#group_name").html("Group: " + group);

  $.ajax({
    url: '/group/' + group,
    timeout: 3000,
    error: function(e) {
      $("#grid").trigger("renderComplete");
    },
    fail: function(e) {
      $("#grid").trigger("renderComplete");
    },
    success: function(d) {
      iot.grid.nodes = d;

//      $("#num_nodes").html(iot.grid.nodes.length);
      $("#grid").trigger("renderStart");
    }
  });  
};

/* q_get_nodes():
 *   part of the node pull system for the grid refresh
 */
iot.q_get_nodes = function(gateway) {
  $.ajax({
    url: "/nwk/" + gateway + "/node",
    timeout: 3000, /* if the poll takes longer than 3 seconds, just call it a failed attempt */
    fail: function(d) {
      console.log("q_get_nodes(): fail()");
      
      --iot.grid.q;

      if ((iot.grid.q === 0) && (iot.grid.refreshing == true)) {
        iot.populate_nodes();
      }
    },
    error: function(xhr, txt, message) {
      if (txt === "timeout") {
        console.log("q_get_nodes(): timeout(" + gateway + ")");
      } else
        console.log("q_get_nodes(): error()");
      
      --iot.grid.q;

      if ((iot.grid.q === 0) && (iot.grid.refreshing == true)) {
        iot.populate_nodes();
      }
    },
    success: function(d) {
      if (!d) {
        console.log("q_get_nodes(): success reached but empty return field");
        return false;
      }

      /* place the nodes for the respective gateway under its own struct */
      for (var i = 0; i < iot.networks.length; i++) {
        if (iot.networks[i]["_id"] == gateway)
          iot.networks[i]["nodes"] = d;
      }
      
      --iot.grid.q;
      
      if ((iot.grid.q == 0) && (iot.grid.refreshing == true)) {
        iot.populate_nodes();
      }
    }
  });
};

iot.grid_refresh = function() {
  if (iot.grid.refreshing === true) {
    alert("Currently refreshing.  Please wait for the current refresh to be complete before trying again.");
    return;
  }
  
  if (iot.networks.length <= 0) {
    alert("Refresh attempt made, but the network list is currently empty.");
    return;
  }
  
//  var unique_gateways = iot.get_unique_networks();
//  iot.grid.refreshing = true;
//  
//  for (var i = 0; i < unique_gateways.length; i++) {
//    iot.grid.q++;
//    iot.q_get_nodes(unique_gateways[i]);
//  }

  iot.populate_nodes();
};

iot.grid_refresh_complete = function() {
  iot.grid.refreshing = false;
  if (iot.grid.q !== 0) {
    console.log("iot.grid.q != 0");
  }
};

iot.grid_render = function() {
  $("#paginator").paging(iot.grid.nodes.length, {
    format: '< nncnn! >',
    perpage: 10,
    lapping: 0,
    page: 1,
    onSelect: function(page) {
      var buf = '';
      var n = 0;
      
      buf += '<table>';
      buf += '<tr id="grid_header" class="grid_header grid_row">';

      buf += iot.grid_header();

      buf += '</tr>';
      

      for (var i = (page * 10) - 10; i < (page * 10); i++) {
        if (!iot.grid.nodes[i])
          continue;
        
        var network_index = iot.get_network_index_by_gateway(iot.grid.nodes[i]["network"]);
        var x = -1;
        
        while ((x < 0) && ((x > iot.grid.backup.length) - 1))
          x = Math.floor(Math.random() * (iot.grid.backup.length));

        /* block for initializing things not listed in the api for testing/development */
		/*
        iot.grid.nodes[i]["group_name"] = "Group 1";
        iot.grid.nodes[i].status["pwr_flgs"] = iot.grid.backup[x]["status"];
        iot.grid.nodes[i].sensor["temp"] = iot.grid.backup[x]["temperature"];
        iot.grid.nodes[i].sensor["light"] = iot.grid.backup[x]["luminosity"];
        iot.grid.nodes[i].sensor["sound"] = iot.grid.backup[x]["sound"];
        iot.grid.nodes[i].sensor["hum"] = iot.grid.backup[x]["humidity"];
        iot.grid.nodes[i].sensor["pres"] = iot.grid.backup[x]["motion"];
        iot.grid.nodes[i].status["pwr_lvl"] = iot.grid.backup[x]["power"];
        iot.grid.nodes[i].status["actuator_light"] = iot.grid.backup[x]["actuator_light"];
        iot.grid.nodes[i].status["actuator_gpio"] = iot.grid.backup[x]["actuator_gpio"];
		*/
		
		//change by elias: not sure where the numbers in the backup came from but chaging it a bit to give numbers between 0 and 4 for now
		//x = Math.floor(Math.random() * 5);
        iot.grid.nodes[i]["group_name"] = "Group 1";
        iot.grid.nodes[i].status["pwr_flgs"] = Math.floor(Math.random() * 5);
        iot.grid.nodes[i].sensor["temp"] = Math.floor(Math.random() * 5);
        iot.grid.nodes[i].sensor["light"] = Math.floor(Math.random() * 5);
        iot.grid.nodes[i].sensor["sound"] = Math.floor(Math.random() * 5);
        iot.grid.nodes[i].sensor["hum"] = Math.floor(Math.random() * 5);
        iot.grid.nodes[i].sensor["pres"] = Math.floor(Math.random() * 5);
        iot.grid.nodes[i].status["pwr_lvl"] = Math.floor(Math.random() * 101);
        iot.grid.nodes[i].status["actuator_light"] = 1;
        iot.grid.nodes[i].status["actuator_gpio"] = 0;

        buf += '<tr class="grid_row" href="details.html?gateway=' + iot.grid.nodes[i]["network"] + '&node=' + iot.grid.nodes[i]["_id"] + '">';

        buf += "<td>"; /* status */
        buf += '<table><tr><td>';
        buf += iot.get_status('status', iot.grid.nodes[i].status["pwr_flgs"]);
        buf += '</td></tr><tr><td class="grid_info">';
        buf += '<div title="Last Updated: ' + new Date(iot.grid.nodes[i]["timestamp"]).toString() + '">';
        buf += iot.get_elapsed_time(iot.grid.nodes[i]["timestamp"]);
        buf += '</div>';
        buf += '</td></tr></table>';
        buf += '</td>'; /* status */

        buf += "<td>"; /* nodes */
        buf += '<table><tr><td>';
        buf += '<a href="details.html?gateway=' + iot.grid.nodes[i]["network"] + '&id=' + iot.grid.nodes[i]["_id"] + '">';
        buf += iot.grid.nodes[i]["name"];
       		// + ' - '
         //     + iot.grid.nodes[i].info["hw_ver"] + '.'
         //     + iot.grid.nodes[i].info["sw_ver"] + ':'
         //     + iot.grid.nodes[i].info["api_ver"];
        buf += '</a>';
        buf += '</td></tr><tr><td class="grid_info">';
        buf += '<table><tr><td>';
        buf += 'GW: ' + iot.networks[network_index].info["netname"];
        buf += '</td></tr><tr><td>';
        buf += 'EUI: ' + iot.grid.nodes[i]["_id"];
        buf += '</td></tr><tr><td>';
        buf += 'PNT: ' + iot.grid.nodes[i].info.rtg_up_pnt[0]["addr"];
        buf += '</td></tr></table>';
        buf += '</td></tr></table>';
        buf += "</td>"; /* --nodes */

        buf += "<td>"; /* sensors */
        buf += '<table><tr><td>';
        buf += iot.get_status('temperature', iot.grid.nodes[i].sensor["temp"]);
        buf += '</td><td>';
        buf += iot.get_status('luminosity', iot.grid.nodes[i].sensor["light"]);
        buf += '</td><td>';
        /* sound is not listed in the API, so dunno how to connect it */
        buf += iot.get_status('sound', iot.grid.nodes[i].sensor["sound"]);
        buf += '</td><td>';
        buf += iot.get_status('humidity', iot.grid.nodes[i].sensor["hum"]);
        buf += '</td><td>';
        /* i'm assuming "pressure" is motion based on the wireframes */
        buf += iot.get_status('motion', iot.grid.nodes[i].sensor["pres"]);
        buf += '</td></tr><tr><td class="grid_info">';
        buf += '<table><tr><td>';
        buf += 'Temp.';
        buf += '</td></tr>';
        buf += '<tr><td>';
        buf += iot.grid.nodes[i].sensor["temp"] + ' (&deg;C)';
        buf += '</td></tr></table>';
        buf += '</td><td class="grid_info">';
        buf += '<table><tr><td>';
        buf += 'Brightness';
        buf += '</td></tr>';
        buf += '<tr><td>';
        buf += iot.grid.nodes[i].sensor["light"] + ' (cd)';
        buf += '</td></tr></table>';

        buf += '</td><td class="grid_info">';
        buf += '<table><tr><td>';
        buf += 'Sound';
        buf += '</td></tr>';
        buf += '<tr><td>';
        buf += iot.grid.nodes[i].sensor["sound"] + ' (dB)';
        buf += '</td></tr></table>';

        buf += '</td><td class="grid_info">';
        buf += '<table><tr><td>';
        buf += 'Humidity';
        buf += '</td></tr>';
        buf += '<tr><td>';
        buf += iot.grid.nodes[i].sensor["hum"] + ' (g/kg)';
        buf += '</td></tr></table>';

        buf += '</td><td class="grid_info">';
        buf += '<table><tr><td>';
        buf += 'Motion';
        buf += '</td></tr>';
        buf += '<tr><td>';
        buf += iot.grid.nodes[i].sensor["pres"] + ' (m/s)';
        buf += '</td></tr></table>';

        buf += '</td></tr></table>';
        buf += "</td>"; /* sensors */

        buf += "<td>"; /* power */
        buf += '<div>' + iot.grid.nodes[i].status["pwr_lvl"] + '%</div>';
		buf += '<div class="battery"><div class="battery_meter"><div style="left:' + iot.grid.nodes[i].status["pwr_lvl"] + '%;"></div></div></div>';
        buf += "</td>"; /* power */

        /* the actuator light and gpio states are not part of the API, so
         * i have no clue how to connect them.  i'm assuming they'll end up
         * in sensors or status, so i just put status for now
         */
        buf += "<td>"; /* actuator */
        buf += '<table><tr><td>';
        buf += iot.get_status('actuator_light', iot.grid.nodes[i].status["actuator_light"]);
        buf += '</td><td>';
        buf += iot.get_status('actuator_gpio', iot.grid.nodes[i].status["actuator_gpio"]);
        buf += '</td></tr><tr><td class="grid_info">';
        buf += 'Light';
        buf += '</td><td class="grid_info">';
        buf += 'GPIO';
        buf += '</td></tr></table>';
        buf += "</td>"; /* actuator */

        buf += "</tr></a>";
      }

      buf += "</table>";

      $("#grid").html(buf);
    },
    onFormat: function(type) {
      switch (type) {
        case 'block':
          if (!this.active)
            return '<a href="#"><span class="pagenum inactive">' + this.value + '</span></a>';
          else if (this.value != this.page)
            return '<a href="#"><span class="pagenum active">' + this.value + '</span></a>';
          else
            return '<a href="#"><span class="pagenum current">' + this.value + '</span></a>';
        case 'next':
          if (this.active)
            return '<a href="#"><span class="pagenum active">&gt;</span></a>';
          else
            return '<a href="#"><span class="pagenum inactive">&gt;</span></a>';
        case 'prev':
          if (this.active)
            return '<a href="#"><span class="pagenum active">&lt;</span></a>';
          else
            return '<a href="#"><span class="pagenum inactive">&lt;</span></a>';
        case 'first':
          if (this.active)
            return '<a href="#"><span class="pagenum active">First</span></a>';
          else
            return '<a href="#"><span class="pagenum inactive">First</span></a>';
        case 'last':
          if (this.active)
            return '<a href="#"><span class="pagenum active">Last</span></a>';
          else
            return '<a href="#"><span class="pagenum inactive">Last</span></a>';
        default:
          return '<a href="#"><span class="pagenum">?</span></a>';
      }
    }
  });
  
  $("#grid").trigger("renderComplete");
};

iot.grid_render_complete = function() {
  $("#grid").trigger("refreshComplete");
};

iot.toggle_menu = function() {
  if ($("#main_container").width() < 960)
    $("#navbar").toggleClass("navbar_open");
};

iot.add_menu = function() {
  $("#main").attr("class", "col-xs-10 main");
  $("#navbar").removeClass("navbar_open");
};

iot.remove_menu = function() {
  $("#main").attr("class", "col-xs-12 main");
};

iot.navbar_group = function(group) {
  var buf = '';
  
  buf += '<a href="group.html?group=' + group["name"] + '">';
  buf += '<div class="nav_element primary nav_group">';
  buf += group["name"] + '<div class="quantity">' + group["count"] + '</div>';
  buf += '</div>';
  buf += '</a>';
  
  return buf;
};

iot.navbar_groups = function() {
  $.ajax({
    url: '/group',
    error: function(e) {
      console.log("error");
    },
    fail: function(e) {
      console.log("fail");
    },
    success: function(d) {
      iot.groups = d;
      var buf = '';

      for (var i = 0; i < iot.groups.length; i++)
        buf += iot.navbar_group(iot.groups[i]);
      
      $("#groups").html(buf);
    }
  });
};

iot.toggle_manage_targets = function() {
  if (iot.active.manage_targets) {
    $("#manage_targets").html('');
    iot.active.manage_targets = false;
  } else {
    var buf = '';

    buf += '<a href="manage_gateway.html">';
    buf += '<div class="nav_element child">';
    buf += 'Gateway';
    buf += '</div>';
    buf += '</a>';
    
    buf += '<a href="manage_nodes.html">';
    buf += '<div class="nav_element child">';
    buf += 'Nodes';
    buf += '</div>';
    buf += '</a>';
    
    buf += '<a href="manage_groups.html">';
    buf += '<div class="nav_element child">';
    buf += 'Groups';
    buf += '</div>';
    buf += '</a>';
    
    $("#manage_targets").html(buf);
    iot.active.manage_targets = true;
  }
};

iot.update_num_connected = function() {
  $.ajax({
    url: '/nwk/any/count',
    timeout: 3000,
    error: function(e) {
      
    },
    fail: function(e) {
      
    },
    success: function(d) {
      $("#num_nodes").html(d["count"]);
    }
  });
};

/* dependant binds are those that require certain dom elements to exist before they can
 * be bound.  this is just an encapsulation to make managing them easier
 */
iot.init_dependant_binds = function() {
  /* working on a way to give the entire row the ability to be a link without resorting
   * to stupid methods like anchor tags inside each <td> or other retardedness
   *   -- a work in progress --
   */
  $(".grid_row").on("click", function() {
    if ($(this).data("href") !== undefined)
      window.document.location = $(this).attr("href");
  });

  $("#grid").on("refreshComplete", function(event) {
    event.preventDefault();
    iot.grid_refresh_complete();
  });
  $("#grid").on("renderComplete", function(event) {
    event.preventDefault();
    iot.grid_render_complete();
  });
  $("#grid").on("renderStart", function(event) {
    event.preventDefault();
    iot.grid_render();
  });
  $("#grid").on("refreshStart", function(event) {
    event.preventDefault();
    iot.grid_refresh();
  });
};

/* independant binds are binds that are predicated on the actual html file.
 * in other words, the elements being bound to are hard coded into the html file
 * so we can be certain that by running this function in .ready they're already
 * created and ready to go.  (also just an ease of flow encapsulation though)
 */
iot.init_independant_binds = function() {
  $("#menu_button").on("click", function(event) {
    event.preventDefault();
    iot.toggle_menu();
  });
  
  $("#manage").on("click", function(event) {
    event.preventDefault();
    iot.toggle_manage_targets();
  });
  
  $(window).on("resize", function(event) {
    if ($("#main_container").width() >= 960)
      iot.add_menu();
    else
      iot.remove_menu();
  });
};

iot.set_nav_active = function(){
	var thisPage = window.location.href.substr(window.location.href.lastIndexOf('/')+1);
	$('#navbar').find('a[href="'+thisPage+'"]').find('.nav_element').addClass('active');
};

jQuery(document).ready(function() {
  iot.navbar_groups();
  iot.set_nav_active();
  iot.get_networks();
  
  if ($("#main_container").width() >= 960)
    iot.add_menu();
  else
    iot.remove_menu();  

  iot.init_independant_binds();
});