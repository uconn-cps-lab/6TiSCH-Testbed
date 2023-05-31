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
window.iot = iot;
iot.grid = {};
iot.grid.refreshing = false;
iot.grid.q = 0;
iot.grid.filter = "all";
iot.grid.nodes = new Array();
iot.grid.backup = new Array();

iot.networks = new Array();
iot.groups = new Array();

iot.group_checkall = false;

iot.network_map = { };

iot.network_map_bounds;
iot.network_map_markers = new Array();
iot.network_map_marker_selection = -1;

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

iot.set_grid_filter = function(filter, firstpass) {
  firstpass = (typeof firstpass === "undefined") ? false : firstpass;
  var gateway = '';
  
  if (!firstpass)
    if (iot.grid.filter == filter)
      return false;
  
  iot.grid.filter = filter;
  $('select[id="gateway"] option[value="' + iot.grid.filter + '"]').prop("selected", true);
  
  if (filter == "all")
    gateway = iot.networks[0]["_id"];
  else if (!isNaN(filter))
    gateway = iot.networks[filter]["_id"];
  
  $("#node_map").attr("href", 'map.html?gateway=' + gateway);
  $("#grid").trigger("refreshStart");
};

iot.get_coordinates = function(gateway) {
  if (!(gateway.info.gps.length > 0))
    return false;
  
  return gateway.info.gps.split(",");
};

iot.load_network_map = function() {
  if (!iot.networks) {
    alert("load_network_map(): networks variable non-existant");
    return;
  }
  
  iot.network_map = new google.maps.Map(document.getElementById("network_map"), {
    center: new google.maps.LatLng(0, 0),
    zoom: 13,
    disableDefaultUI: true
  });
  
  iot.network_map_bounds = new google.maps.LatLngBounds();
  
  for (var i = 0; i < iot.networks.length; i++) {
    var coords = iot.get_coordinates(iot.networks[i]);
    
    if (coords) /* only create the marker if we actually pulled the gps info */
      iot.network_map_markers.push(new google.maps.Marker({
        position: new google.maps.LatLng(coords[0], coords[1]),
        map: iot.network_map,
        title: iot.networks[i].info.netname,
        id: iot.networks[i]._id,
        index: i
      }));
  }
  
    for (var i = 0; i < iot.network_map_markers.length; i++) {
    google.maps.event.addListener(iot.network_map_markers[i], "click", function() {
      iot.network_map_marker_selection = this.index;

      iot.network_map.panTo(this.position);

      google.maps.event.trigger(iot.network_map, 'resize');

      $("#network_map_modal").modal("hide");
      
      iot.set_grid_filter(this.index); /* this pushes a grid refresh */
    });
    
    iot.network_map_bounds.extend(iot.network_map_markers[i].position);
  }
  
  iot.network_map.setCenter(iot.network_map_bounds.getCenter());
  iot.network_map.panToBounds(iot.network_map_bounds);
  iot.network_map.panBy(-180, -150);

//  iot.network_map.setZoom(13);
//  iot.network_map.panTo(iot.network_map_bounds.getCenter());
//  iot.network_map.fitBounds(iot.network_map_bounds);
  google.maps.event.trigger(iot.network_map, 'resize');
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
      // $.ajax({
      //   url: "node/info.json",
      //   success: function(backup_data) {
      //     iot.grid.backup = backup_data;


      //   }
      // });

          iot.grid_ui();
          //iot.load_network_map();

          iot.init_dependant_binds();


          /* pushing a refresh event for the grid here, it's ensured that we have
           * already loaded the network (gateways) and have successfully
           * loaded in my bs dummy data and the ui, so now it's time to
           * push our initial grid render, so we make sure "all nodes" is
           * selected intiially, and do it
           */
          $('select[id="gateway"] option[value="all"]').prop("selected", true);
//          $("#grid").trigger("refreshStart");
          iot.set_grid_filter("all", true);
          iot.update_num_connected();
			
    }
  });
};

iot.grid_header = function(dropdown) {
  var buf = '';
  
  buf += '<td class="col_checkbox"><input id="add_all" type="checkbox" name="add_all" value="all" /></td>';
  
  if (dropdown) {
    buf += '<td id="add_to_group_dropdown" class="col_add_to_group" colspan="5">';
    buf += '<div class="dropdown">';
    buf += '<a data-toggle="dropdown" href="#">Add to Group<span class="caret"></span></a>';
    buf += '<ul class="dropdown-menu" role="menu" aria-labelledby="add_to_groups">';
    
    for (var i = 0; i < iot.groups.length; i++) {
      buf += '<li class="add_to_group" value="' + iot.groups[i]["name"] + '">' + iot.groups[i]["name"] + '</li>';
    }
    
    buf += '<li id="manage_groups" value="manage">Manage Groups</li>';
    
    buf += '</ul>';
    buf += '</div>';
    buf += '</td>';
  } else {
    buf += '<td class="col_status">Status</td>';
    buf += '<td class="col_node">Node</td>';
    buf += '<td class="col_sensors">Sensors</td>';
    buf += '<td class="col_power">Power</td>';
    buf += '<td class="col_actuator">Actuator</td>';
  }

  return buf;
};

iot.grid_ui = function() {
  if (iot.networks.length > 1) {
    $("#gateways").html(iot.grid_ui_gateways());
    
    var n = 0;
    
    for (var i = 0; i < iot.networks.length; i++)
      if (iot.networks[i].info["gps"].length > 0)
        n++;
    
    if (n > 1)
      $("#map").html(iot.grid_ui_map());
  }
};

iot.grid_ui_map = function() {
  return '<a id="open_network_map_modal" class="open_network_map_modal"></a>';
};

iot.grid_ui_gateways = function() {
  var buf = '';
  
  buf += '<label for="gateway">Show Gateway:&nbsp;</label>';
  buf += '<select id="gateway" name="gateway" class="dropdown">';
  buf += '<option value="all" selected="selected">All Gateways</option>';
  
  for (var i = 0; i < iot.networks.length; i++)
    buf += '<option value="' + i + '">' + iot.networks[i].info["netname"] + '</option>';

  buf += '</select>';
  
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
    case 'pressure':
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

/* populate_nodes():
 *   filter: param taken from the gateways dropdown.
 *           "all" - add in all nodes from all available gateways
 *           # - if filter is a number, it refers to the index of iot.networks[index]
 *           undefined: same as all
 */
iot.populate_nodes = function(filter) {
  /* remove all nodes from the node list so we don't have strays in our new grid render
   * (popping preserves references, but we don't have any here, so we do it the faster way)
   */
  iot.grid.nodes = [];
  
//  while (iot.grid.nodes.length)
//    iot.grid.nodes.pop();
  
  if ((filter === undefined) || (filter == "all")) {
    for (var i = 0; i < iot.networks.length; i++)
      for (var j = 0; j < iot.networks[i].nodes.length; j++)
        iot.grid.nodes.push(iot.networks[i].nodes[j]);
  } else {
    for (var i = 0; i < iot.networks[filter].nodes.length; i++)
      iot.grid.nodes.push(iot.networks[filter].nodes[i]);
  }
  
//  $("#num_nodes").html(iot.grid.nodes.length); /* .length now represents the number of nodes, filtered by gateway */
  $("#grid").trigger("renderStart");
};

/* get_nodes():
 *   acquire and set the nodes for a single gateway
 */
iot.get_nodes = function(gateway) {
  $.ajax({
    url: "/nwk/" + gateway + "/node",
    timeout: 3000, /* if the poll takes longer than 3 seconds, just call it a failed attempt */
    fail: function(d) {
      console.log("get_nodes(): fail()");
    },
    error: function(xhr, txt, message) {
      if (txt === "timeout")
        console.log("get_nodes(): timeout(" + gateway + ")");
      else
        console.log("get_nodes(): error()");
    },
    success: function(d) {
      if (!d)
        return false;
      
      for (var i = 0; i < iot.networks.length; i++)
        if (iot.networks[i]["_id"] == gateway)
          iot.networks[i]["nodes"] = d;
    }
  });
};

/* q_get_nodes():
 *   part of the node pull system for the grid refresh
 */
iot.q_get_nodes = function(gateway) {
  $.ajax({
    url: "/nodes",
    timeout: 3000, /* if the poll takes longer than 3 seconds, just call it a failed attempt */
    fail: function(d) {
      console.log("q_get_nodes(): fail()");
      
      --iot.grid.q;

      if ((iot.grid.q === 0) && (iot.grid.refreshing == true)) {
        iot.populate_nodes(iot.grid.filter);
      }
    },
    error: function(xhr, txt, message) {
      if (txt === "timeout") {
        console.log("q_get_nodes(): timeout(" + gateway + ")");
      } else
        console.log("q_get_nodes(): error()");
      
      --iot.grid.q;

      if ((iot.grid.q === 0) && (iot.grid.refreshing == true)) {
        iot.populate_nodes(iot.grid.filter);
      }
    },
    success: function(d) {
      if (!d) {
        console.log("q_get_nodes(): success reached but empty return field");
        return false;
      }
      
      for (var i = 0; i < iot.networks.length; i++) {
        if (iot.networks[i]["_id"] == gateway)
          iot.networks[i]["nodes"] = d;
      }
      
      --iot.grid.q;
      
      if ((iot.grid.q == 0) && (iot.grid.refreshing == true)) {
        iot.populate_nodes(iot.grid.filter);
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
  
  iot.grid.refreshing = true;
  iot.grid.q = iot.networks.length;
  
  for (var i = 0; i < iot.networks.length; i++)
    iot.q_get_nodes(iot.networks[i]["_id"]);
};

iot.grid_refresh_complete = function() {
  iot.grid.refreshing = false;
  if (iot.grid.q !== 0) {
    console.log("iot.grid.q != 0");
  }
  iot.init_actuator_binds();
};

iot.grid_render = function() {
  $("#paginator").paging(iot.grid.nodes.length, {
    format: '< nncnn! >',
    perpage: 100,
    lapping: 0,
    page: 1,
    onSelect: function(page) {
      var buf = '';
      var n = 0;
      
      buf += '<table>';
      buf += '<form id="form_add_to_group" action="#" method="PUT">';
      buf += '<tr id="grid_header" class="grid_header grid_row">';

      buf += iot.grid_header(false);

      buf += '</tr>';
      

      for (var i = (page * 100) - 100; i < (page * 100); i++) {
        if (!iot.grid.nodes[i])
          break;
        
		
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
        iot.grid.nodes[i].status={};
        iot.grid.nodes[i].sensor={};
        iot.grid.nodes[i].sensor["temp"] = 1;
        iot.grid.nodes[i].sensor["light"] = 1;
        iot.grid.nodes[i].sensor["sound"] = 1;
        iot.grid.nodes[i].sensor["hum"] = 1;
        iot.grid.nodes[i].sensor["pressure"] = 1;
        iot.grid.nodes[i].sensor["motion"] = 1;
        iot.grid.nodes[i].status["actuator_light"] = 1;
        iot.grid.nodes[i].status["actuator_gpio"] = 0;
        if(iot.grid.nodes[i].eui64==null)iot.grid.nodes[i].eui64="unknown"
        if(iot.grid.nodes[i].address==null)iot.grid.nodes[i].address="unknown"
        if(iot.grid.nodes[i].network==null)iot.grid.nodes[i].network="00:00:00:00:00:00"
        if(iot.grid.nodes[i].sensors==null){
          iot.grid.nodes[i].status["pwr_lvl"] = "Unknown";
        }else{
          iot.grid.nodes[i].status["pwr_lvl"] = iot.grid.nodes[i].sensors["bat"].toString() + "V";
		}
        if(iot.grid.nodes[i].lifetime<=0){
		  iot.grid.nodes[i].status["pwr_flgs"] = 0;
		}else{
		  iot.grid.nodes[i].status["pwr_flgs"] = 1;
		}
        if (iot.grid.nodes[i].meta["type"] == null)
        {
          iot.grid.nodes[i].meta["type"]="Unknown";
        }

        buf += '<tr id="' + iot.grid.nodes[i]["_id"] + '" class="grid_row" href="details.html?gateway=' + iot.grid.nodes[i]["network"] + '&node=' + iot.grid.nodes[i]["_id"] + '">';

        buf += '<td class="col_checkbox">'; /* group */
        buf += '<input type="checkbox" name="add_to_group" value="' + iot.grid.nodes[i]._id + '" />';
        buf += "</td>"; /* group */

        buf += '<td class="col_status">'; /* status */
        buf += '<table><tr><td class = "node_delete">';
        buf += iot.get_status('status', iot.grid.nodes[i].status["pwr_flgs"]);
        buf += '</td></tr><tr><td class="grid_info">';
        //buf += '<div title="Last Updated: ' + new Date(iot.grid.nodes[i]["timestamp"]).toString() + '">';
        //buf += iot.get_elapsed_time(iot.grid.nodes[i]["timestamp"]);
        //buf += '</div>'
        buf += '</td></tr></table>';
        buf += '</td>'; /* status */

        buf += '<td class="col_node">'; /* nodes */
        buf += '<table><tr><td>';
        buf += '<a href="details.html?gateway=' + iot.grid.nodes[i]["network"] + '&id=' + iot.grid.nodes[i]._id + '">';
        buf += iot.grid.nodes[i]._id; 
          //    + ' - '
          //    + iot.grid.nodes[i].info["hw_ver"] + '.'
          //    + iot.grid.nodes[i].info["sw_ver"] + ':'
          //    + iot.grid.nodes[i].info["api_ver"];
        buf += '</a>';
        buf += '</td></tr><tr><td class="grid_info">';
        buf += 'EUI: ' + iot.grid.nodes[i].eui64 + ' - ADDR: ' + iot.grid.nodes[i].address+ ' Node Type:'+iot.grid.nodes[i].meta["type"];
        buf += '</td></tr></table>';
        buf += "</td>"; /* --nodes */

        buf += '<td class="col_sensors">'; /* sensors */
        buf += '<table><tr><td>';
        buf += iot.get_status('temperature', iot.grid.nodes[i].sensor["temp"]);
        buf += '</td><td>';
        buf += iot.get_status('luminosity', iot.grid.nodes[i].sensor["light"]);
        buf += '</td><td>';
        buf += iot.get_status('pressure', iot.grid.nodes[i].sensor["pressure"]);
        buf += '</td><td>';
        buf += iot.get_status('humidity', iot.grid.nodes[i].sensor["hum"]);
        buf += '</td><td>';
        buf += iot.get_status('motion', iot.grid.nodes[i].sensor["motion"]);
        buf += '</td></tr><tr><td class="grid_info">';
        buf += 'Temp.';
        buf += '</td><td class="grid_info">';
        buf += 'Brightness';
        buf += '</td><td class="grid_info">';
        buf += 'Pressure';
        buf += '</td><td class="grid_info">';
        buf += 'Humidity';
        buf += '</td><td class="grid_info">';
        buf += 'Motion';
        buf += '</td></tr></table>';
        buf += "</td>"; /* sensors */

        buf += '<td class="col_power node_power">'; /* power */
        buf += '<div>' + iot.grid.nodes[i].status["pwr_lvl"] + '</div>';
		buf += '<div class="battery"><div class="battery_meter"><div style="left:' + 100 + '%;"></div></div></div>';
//		buf += '<div class="battery"><div class="battery_meter"><div style="left:' + iot.grid.nodes[i].status["pwr_lvl"] + '%;"></div></div></div>';
        buf += "</td>"; /* power */

        /* the actuator light and gpio states are not part of the API, so
         * i have no clue how to connect them.  i'm assuming they'll end up
         * in sensors or status, so i just put status for now
         */
        buf += '<td class="col_actuator">'; /* actuator */
        buf += '<table><tr><td class="actuator_light_toggle">';
        buf += iot.get_status('actuator_light', iot.grid.nodes[i].status["led"]);
        buf += '</td>';
       // buf += iot.get_status('actuator_gpio', iot.grid.nodes[i].status["actuator_gpio"]);
        buf += '</tr><tr><td class="grid_info">';
        buf += 'LED</td>';
        //buf += '</td><td class="grid_info">';
        //buf += 'GPIO';
        //buf += '</td></tr></table>';
        buf += '</tr></table>';        
        buf += "</td>"; /* actuator */

        buf += "</tr></a>";
      }

      buf += '</form>';
      buf += "</table>";

      $("#grid").html(buf);
      
      $('#add_all').on("change", iot.on_add_all);
      $('input[name="add_to_group"]').on("change", iot.on_add_group_change);
      iot.init_actuator_binds();
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

iot.on_add_all = function() {
  if (this.checked) {
    $('input[name="add_to_group"]').prop("checked", true);
    iot.group_checkall = true;
  } else {
    $('input[name="add_to_group"]').prop("checked", false);
    iot.group_checkall = false;
  }
  
  /* trigger a change on a checkbox element to force a grid_header update */
  $('input[name="add_to_group"]:first').trigger("change");
};

iot.on_add_group_change = function() {
  if ($('input[name="add_to_group"]:checked').fieldValue().length) {
    $("#grid_header").html(iot.grid_header(true));
    iot.init_add_to_group_binds();
  } else
    $("#grid_header").html(iot.grid_header(false));
  
  /* #add_all was deleted and recreated by grid_header(), so let's see if the
   * last change to it was checking or unchecking it
   */
  if (iot.group_checkall)
    $("#add_all").prop("checked", true);
  else
    $("#add_all").prop("checked", false);
  
  /* rebind the since deleted on->change event */
  $('#add_all').on("change", iot.on_add_all);
};

iot.grid_render_complete = function() {
  $("#grid").trigger("refreshComplete");
};

iot.toggle_menu = function() {
  if ($("#main_container").width() < 960)
    $("#navbar").toggleClass("navbar_open");
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

iot.modal_gateway_list = function() {
  var buf = '';
  
  buf += '<ul>';
  
  for (var i = 0; i < iot.networks.length; i++) {
    if (i == iot.grid.filter)
      buf += '<li class="selected" value="' + i + '"><table><tr><td width="24px"><div class="fa fa-map-marker fa-lg"></div></td><td>';
    else
      buf += '<li value="' + i + '"><table><tr><td width="24px"><div class="fa fa-map-marker fa-lg icon_hidden"></div></td><td>';

    buf += iot.networks[i].info["netname"];
    buf += '</td></tr></table></li>';
  }
  
  buf += '</ul>';

  return buf;
};

/* these binds have to be specially handled and bound repeatedly because the
 * add to group dropdown is dynamically pushed/popped out of existance
 */
iot.init_add_to_group_binds = function() {
  $('.add_to_group').on("click", function(event) {
    event.preventDefault();
    event.stopPropagation();
    
    var eui_list = [];
    var group = $(this).attr('value');
    
    $('input[name="add_to_group"]:checked').each(function(index, element) {
      eui_list.push($(element).attr('value'));
    });
    
    if (eui_list.length >= 1) {
      $.ajax({
        url: '/node/group/' + group,
        type: 'put',
        data: {
          "euis": eui_list
        },
        error: function(e) {

        },
        fail: function(e) {

        },
        success: function(d) {
          iot.navbar_groups();
          $("#status_message").html('Selected nodes added to group "' + group + '"');
          $("#status_message").removeClass("hidden");

          $(":checked").each(function() {
            this.checked = false;
          });
          
          $('input[name="add_to_group"]:first').trigger("change");
        }
      });
    }
  });
  
  $("#manage_groups").on("click", function(event) {
    event.preventDefault();
    
    window.location.href = 'manage_groups.html';
  });
};

iot.update_num_connected = function() {
  $.ajax({
    url: '/nodes/count',
    timeout: 3000,
    error: function(e) {
      
    },
    fail: function(e) {
      
    },
    success: function(d) {
      $("#num_nodes").html(d);
    }
  });
};


iot.init_actuator_binds = function () {

  $(".actuator_light_toggle").on("click", function() {

  	var clicked_id = $(this).closest(".grid_row").attr('id');
  	var toggle, toggle_rev; 

  	if($(this).find(".gridicon").hasClass('act_on')) {
  		toggle = 0;
  		toggle_rev = 1;
  	} else {
  		toggle = 1;
  		toggle_rev = 0;
  	}
  	
  	$(this).html(iot.get_status('actuator_light', toggle));

	$.ajax({
        url: '/nodes/'+clicked_id+'/led/' + toggle ,
        type: 'put',
        success: function(d) {
        }
	});

   });

  $(".node_delete").on("click", function() {
  	var clicked_id = $(this).closest(".grid_row").attr('id');
  	if(confirm("Delete node "+clicked_id+" ?")){
      $.ajax({
            url: '/nodes/'+clicked_id ,
            type: 'delete',
            success: function(d) {
              window.location.reload(true);
            }
      });
    }
  })
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

  $("#open_network_map_modal").on("click", function() {
    $("#network_map_modal").modal("toggle");

    google.maps.event.trigger(iot.network_map, 'resize');

    $("#network_map_list").html(iot.modal_gateway_list());

    /* this bind has to be located after the map_list set.  i renew the
     * list each call.  not really a good method, would be better to
     * play with the .selected class on the li and set/unset and what
     * not, but for right now it works, i'll deal with the overhead
     * later 
     */
    $("#network_map_list ul li").on("click", function(event) {
      event.preventDefault();
      $("#network_map_modal").modal("hide");
      iot.set_grid_filter($(this).val());
    });
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

  $('select[id="gateway"]').on("change", function() {
    iot.set_grid_filter($(this).val()); /* set_grid_filter pushes the refresh trigger normally */
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


};

iot.browser_detection = function(){
	if (navigator.appName == 'Microsoft Internet Explorer') {
		var userAgent = navigator.userAgent,
			re = new RegExp("MSIE ([0-9]{1,}[\.0-9]{0,})"),
			versionIE;
		if (re.exec(userAgent) != null) versionIE = parseFloat(RegExp.$1);
		if (versionIE != undefined && versionIE < 9) iot.compatibility_bar();
	}
}

iot.compatibility_bar = function(){
	var textMsg = 'This Demo has features and/or functionality that will not work in versions 8 and below of Internet Explorer. Please use a newer browser.';
	var mainContainer = document.getElementById("main_container");
	var msgDiv = document.createElement("div");
	msgDiv.innerHTML = textMsg;
	msgDiv.style.background = 'yellow';
	msgDiv.style.fontSize = '20px';
	msgDiv.style.padding = '10px';
	mainContainer.insertBefore(msgDiv, mainContainer.firstChild);
};

iot.set_nav_active = function(){
	var thisPage = window.location.href.substr(window.location.href.lastIndexOf('/')+1);
	$('#navbar').find('a[href="'+thisPage+'"]').find('.nav_element').addClass('active');
};



jQuery(document).ready(function() {
  iot.navbar_groups();
  iot.set_nav_active();

  iot.get_networks();
  
  iot.init_independant_binds();

});
