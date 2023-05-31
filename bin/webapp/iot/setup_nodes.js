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

iot.networks = new Array();
iot.new_nodes = new Array();
iot.q = 0;

/* -------------------------------------------------------------------------- */

iot.get_status = function(type, value) {
  switch (type) {
    /* eventually overall status needs to be replaced with a fallthrough block
     * based upon the values gathered from the other sensors
     */
    case 'status':
      switch (value) {
        case 0:
          return "<span class=\"fa fa-circle gridicon status_ok\"></span>";
        case -1:
          return "<span class=\"fa fa-circle gridicon status_warn\"></span>";
        case -2:
          return "<span class=\"fa fa-circle gridicon status_error\"></span>";
        default:
          return "<span class=\"fa fa-circle gridicon status_inactive\"></span>";
      }
    case 'temperature':
      switch (value) {
        case 0:
          return "<span class=\"fa fa-fire gridicon status_ok\"></span>";
        case -1:
          return "<span class=\"fa fa-fire gridicon status_warn\"></span>";
        case -2:
          return "<span class=\"fa fa-fire gridicon status_error\"></span>";
        default:
          return "<span class=\"fa fa-fire gridicon status_inactive\"></span>";
      }
    case 'luminosity':
      switch (value) {
        case 0:
          return "<span class=\"fa fa-lightbulb-o gridicon status_ok\"></span>";
        case -1:
          return "<span class=\"fa fa-lightbulb-o gridicon status_warn\"></span>";
        case -2:
          return "<span class=\"fa fa-lightbulb-o gridicon status_error\"></span>";
        default:
          return "<span class=\"fa fa-lightbulb-o gridicon status_inactive\"></span>";
      }
    case 'sound':
      switch (value) {
        case 0:
          return "<span class=\"fa fa-volume-up gridicon status_ok\"></span>";
        case -1:
          return "<span class=\"fa fa-volume-up gridicon status_warn\"></span>";
        case -2:
          return "<span class=\"fa fa-volume-up gridicon status_error\"></span>";
        default:
          return "<span class=\"fa fa-volume-up gridicon status_inactive\"></span>";
      }
    case 'humidity':
      switch (value) {
        case 0:
          return "<span class=\"fa fa-tint gridicon status_ok\"></span>";
        case -1:
          return "<span class=\"fa fa-tint gridicon status_warn\"></span>";
        case -2:
          return "<span class=\"fa fa-tint gridicon status_error\"></span>";
        default:
          return "<span class=\"fa fa-tint gridicon status_inactive\"></span>";
      }
    case 'motion':
      switch (value) {
        case 0:
          return "<span class=\"fa fa-cogs gridicon status_ok\"></span>";
        case -1:
          return "<span class=\"fa fa-cogs gridicon status_warn\"></span>";
        case -2:
          return "<span class=\"fa fa-cogs gridicon status_error\"></span>";
        default:
          return "<span class=\"fa fa-cogs gridicon status_inactive\"></span>";
      }
    case 'actuator_light':
      switch (value) {
        case 1:
          return "<span class=\"fa fa-toggle-on gridicon status_ok\"></span>";
        case 0:
          return "<span class=\"fa fa-toggle-off gridicon status_inactive\"></span>";
        case -1:
          return "<span class=\"fa fa-toggle-off gridicon status_warn\"></span>";
        case -2:
          return "<span class=\"fa fa-toggle-off gridicon status_error\"></span>";
        default:
          return "<span class=\"fa fa-toggle-off gridicon status_inactive\"></span>";
      }
    case 'actuator_gpio':
      switch (value) {
        case 1:
          return "<span class=\"fa fa-toggle-on gridicon status_ok\"></span>";
        case 0:
          return "<span class=\"fa fa-toggle-off gridicon status_inactive\"></span>";
        case -1:
          return "<span class=\"fa fa-toggle-off gridicon status_warn\"></span>";
        case -2:
          return "<span class=\"fa fa-toggle-off gridicon status_error\"></span>";
        default:
          return "<span class=\"fa fa-toggle-off gridicon status_inactive\"></span>";
      }
    default:
      break;
  }
};

iot.node_table = function() {
  var buf = '';

  buf += '<table id="node_table">';
  buf += '<tr class="table_header"><td>';
  buf += 'ID';
  buf += '</td><td>';
  buf += 'GATEWAY';
  buf += '</td><td>';
  buf += 'STATUS';
  buf += '</td><td>';
  buf += 'ACTIONS';
  buf += '</td></tr>';
  buf += '</table>';
  
  return buf;
};

iot.add_row = function(node) {
  node = (typeof node === "undefined") ? false : node;

  
  var buf = '';

  if (!node)
    buf = '<tr><td><div class="divider"></div></td><td><div class="divider"></div></td><td><div class="divider"></div></td><td><div class="divider"></div></td></tr>';
  else {
    buf += '<tr>';
    buf += '<td>';
    buf += node["_id"];
    buf += '</td>';
    buf += '<td>';
    buf += node["network"];
    buf += '</td>';
    buf += '<td id="status_' + node["_id"] + '">';
    buf += iot.get_status('status', node.status["pwr_flgs"]);
    buf += '</td>';
    buf += '<td>';
    buf += '<button id="' + node["_id"] + '" type="button" name="connect">Connect</button>';
    buf += '</td>';
    buf += '</tr>';
  }
  
  return buf;
};

iot.get_node_index_by_id = function(id) {
  for (var i = 0; i < iot.new_nodes.length; i++)
    if (iot.new_nodes[i]["_id"] == id)
      return i;
  
  return false;
};

iot.connect_node = function(id) {
  var nid = iot.get_node_index_by_id(id);
  var node = iot.new_nodes[nid];
  var jqn = "#" + id;
  
  $.ajax({
    url: '/nwk/' + node["network"] + '/node/' + node.info["mla"] + '/auth/1',
    method: 'PUT',
    timeout: 3000,
    error: function(e) {
      console.log("error");
      $(jqn).html("Connect");
      $(jqn).prop("disabled", false);
    },
    fail: function(e) {
      console.log("fail");
      $(jqn).html("Connect");
      $(jqn).prop("disabled", false);
    },
    success: function(d) {
      $(jqn).html("Connected");
      $(jqn).prop("disabled", true); /* setting disabled again shouldnt be necessary, but oh well */
      
      jqn = "#status_" + id;
      $(jqn).html(iot.get_status('status', node.status["pwr_flgs"]) + ' Active');
    }
  });
};

iot.create_table = function() {
  $("#node_info").html(iot.node_table());
  
  if (!iot.new_nodes.length) {
    $("#node_table").append(iot.add_row());
    $('button[name="continue"]').prop("disabled", true);
    
    $("#status_message").removeClass("hidden success");
    $("#status_message").addClass("error");
    $("#status_message").html("ERROR:  No nodes were detected on this gateway.  Switch on the nodes and try again.");
  } else {
    var n = 0;

    for (var i = 0; i < iot.new_nodes.length; i++) {
      $("#node_table").append(iot.add_row(iot.new_nodes[i]));
      n++;
    }
    
    $("#status_message").removeClass("hidden error");
    $("#status_message").addClass("success");
    $("#status_message").html("SUCCESS:  " + n + " new nodes have been detected.");
    
    $('button[name="continue"]').prop("disabled", false);
  }
  
  $('button[name="connect"]').on("click", function(event) {
    event.preventDefault();
    $(this).html("Connecting...");
    $(this).prop("disabled", true);
    
    iot.connect_node($(this).attr("id"));
  });
};

iot.get_new_nodes = function(gateway) {
  $.ajax({
    url: '/nwk/' + gateway + '/node/new',
    timeout: 3000,
    error: function(e) {
      console.log("error");
      iot.q--;
      
      if (iot.q === 0)
        iot.create_table();
    },
    fail: function(e) {
      console.log("fail");
      iot.q--;

      if (iot.q === 0)
        iot.create_table();
    },
    success: function(d) {
      iot.q--;
      
      for (var i = 0; i < d.length; i++)
        iot.new_nodes.push(d[i]);
      
      if (iot.q === 0)
        iot.create_table();
    }
  });
};

iot.get_networks = function() {
  $.ajax({
    url: '/nwk',
    timeout: 3000,
    error: function(e) {
      console.log("error");
    },
    fail: function(e) {
      console.log("fail");
    },
    success: function(d) {
      iot.networks = d;
      
      if (!iot.networks.length) {
        alert("Error retrieving network data.");
        return false;
      }
      
      iot.new_nodes = [];
      
      for (var i = 0; i < iot.networks.length; i++) {
        iot.q++;
        iot.get_new_nodes(iot.networks[i]["_id"]);
      }
    }
  });
};

iot.set_nav_active = function(){
	var thisPage = window.location.href.substr(window.location.href.lastIndexOf('/')+1);
	$('#navbar').find('a[href="'+thisPage+'"]').find('.nav_element').addClass('active');
};

jQuery(document).ready(function() {
  iot.get_networks();
  
  $('button[name="refresh"]').on("click", function(event) {
    event.preventDefault();
    iot.get_networks();
  });
  
  $('button[name="continue"]').on("click", function(event) {
    event.preventDefault();
    
    /* continue button is completely user driven.  we don't need to give the
     * user any extra time to read the status message; no need to timeout the
     * redirect.
     */
    window.location.href = '/list.html';
  });
});