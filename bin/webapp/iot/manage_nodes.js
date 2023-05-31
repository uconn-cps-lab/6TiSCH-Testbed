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
iot.connected_nodes = new Array();
iot.groups = new Array();

iot.active = { };
iot.active.manage_targets = false;

iot.num_nodes = 0;
iot.q = 0;

/* -------------------------------------------------------------------------- */
iot.connect_websocket = function() {
  iot.socket = new WebSocket("ws://"+location.host+":5000/");

  iot.socket.onmessage = function(event) {
    var data = JSON.parse(event.data);
    var connected = JSON.parse(data.auth);
//    if (connected == 0) {
      iot.get_networks();
//      location.reload();
//    }
  } 
}

/* -------------------------------------------------------------------------- */

iot.get_status = function(type, value) {
  switch (type) {
    /* eventually overall status needs to be replaced with a fallthrough block
     * based upon the values gathered from the other sensors
     */
    case 'status':
      switch (value) {
        case 0:
          return "<div class='gridicon status_ok'></div>";
        case -1:
          return "<span class=\"fa fa-circle gridicon status_warn\"></span>";
        case -2:
          return "<span class=\"fa fa-circle gridicon status_error\"></span>";
        default:
          return "<div class='gridicon status_inactive'></div>";
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

iot.node_table = function() {
  var buf = '';

  buf += '<table id="node_table">';
  buf += '<tr class="table_header"><td width="40%">';
  buf += 'NAME';
  buf += '</td><td>';
  buf += 'EUI';
  buf += '</td><td>';
  buf += 'DEVICE';
  buf += '</td><td>';
  buf += 'STATUS';
  buf += '</td><td>';
  buf += 'ACTIONS';
  buf += '</td></tr>';
  buf += '</table>';
  
  return buf;
};



iot.add_row = function(node, type) {
  node = (typeof node === "undefined") ? false : node;
  type = (typeof type === "undefined") ? false : type;
  
  var buf = '';

  if (!node)
    buf = '<tr><td><div class="divider"></div></td><td><div class="divider"></div></td><td><div class="divider"></div></td><td><div class="divider"></div></td></tr>';
  else {
    buf += '<tr>';
    buf += '<td>';
    buf += '<span class="fa fa-pencil"></span><div id="' + node["_id"] + '" class="edit_name editable editable-click inline-input">';
    buf += node["name"];
    buf += '</div>';
    buf += '</td>';
    buf += '<td>';
    buf += node["_id"];
    buf += '</td>';
    buf += '<td>';
    buf += node.info["dev"];
    buf += '</td>';
    buf += '<td id="status_' + node["_id"] + '">';
    buf += iot.get_status('status', node.status["pwr_flgs"]);
    buf += '</td>';
    buf += '<td>';
    if (type == "new")
      buf += '<button id="' + node["_id"] + '" type="button" class="connect" name="connect">Connect</button>';
    else if (type == "connected")
      buf += '<button id="' + node["_id"] + '" type="button" class="disconnect" name="disconnect">Disconnect</button>';
    else
      ;
    buf += ' <button id="' + node["_id"] + '" type="button" class="delete" name="delete">Delete</button>';
    buf += '</td>';
    buf += '</tr>';
  }
  
  return buf;
};

iot.get_new_node_index_by_id = function(id) {
  for (var i = 0; i < iot.new_nodes.length; i++)
    if (iot.new_nodes[i]["_id"] == id)
      return i;
  
  return false;
};

iot.get_connected_node_index_by_id = function(id) {
  for (var i = 0; i < iot.connected_nodes.length; i++)
    if (iot.connected_nodes[i]["_id"] == id)
      return i;
  
  return false;
};

iot.connect_node = function(id) {
  var nid = iot.get_new_node_index_by_id(id);
  var node = iot.new_nodes[nid];
  var jqn = 'button#'+id+'.connect';
  
  $.ajax({
    url: '/nwk/' + node["network"] + '/node/' + node["_id"] + '/auth/1',
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

iot.disconnect_node = function(id) {
  var nid = iot.get_connected_node_index_by_id(id);
  var node = iot.connected_nodes[nid];
  var jqn = 'button#'+id+'.disconnect';
  
  $.ajax({
    url: '/nwk/' + node["network"] + '/node/' + node.info["mla"] + '/auth/0',
    method: 'PUT',
    timeout: 3000,
    error: function(e) {
      console.log("error");
      $(jqn).html("Disconnect");
      $(jqn).prop("disabled", false);
    },
    fail: function(e) {
      console.log("fail");
      $(jqn).html("Disconnect");
      $(jqn).prop("disabled", false);
    },
    success: function(d) {
      $(jqn).html("Disconnected");
      $(jqn).prop("disabled", true); /* setting disabled again shouldnt be necessary, but oh well */
      
      jqn = "#status_" + id;
      $(jqn).html(iot.get_status('status', node.status["pwr_flgs"]) + ' Active');
    }
  });
};

iot.delete_node = function(id) {
	if(id=='all') {
		nodeAddr = 'all';
	} else {
		var nid = iot.get_connected_node_index_by_id(id);
		var node = iot.connected_nodes[nid];
		var nodeAddr = node.info["mla"];
	}
  $.ajax({
    url: '/node/' + nodeAddr,
    method: 'DELETE',
    timeout: 3000,
    error: function(e) {
      console.log("error");
    },
    fail: function(e) {
      console.log("fail");
    },
    success: function(d) {
    	iot.get_networks();
    }
  });
};

iot.create_table = function() {
  $("#node_info").html(iot.node_table());

  if (($("#node_filter").val() == "all") || ($("#node_filter").val() == "new")) {
    if (!iot.new_nodes.length) {
      $('button[name="continue"]').prop("disabled", true);

      $("#status_message").removeClass("hidden success");
      $("#status_message").addClass("error");
      $("#status_message").html("No new nodes have been detected.");
    } else {
      var n = 0;

      for (var i = 0; i < iot.new_nodes.length; i++) {
        $("#node_table").append(iot.add_row(iot.new_nodes[i], "new"));
        n++;
      }

      $("#status_message").removeClass("hidden error");
      $("#status_message").addClass("success");
      $("#status_message").html("" + n + " new nodes have been detected.");
    }
  }
  
  if (($("#node_filter").val() == "all") || ($("#node_filter").val() == "connected")) {
    if (iot.connected_nodes.length) {
      for (var i = 0; i < iot.connected_nodes.length; i++) {
        $("#node_table").append(iot.add_row(iot.connected_nodes[i], "connected"));
      }
    }
  }
  
  if (!iot.new_nodes.length && !iot.connected_nodes.length)
    $("#node_table").append(iot.add_row());
  
//  $("#num_nodes").html('' + iot.connected_nodes.length + '+' + iot.new_nodes.length);
//  $("#num_nodes").html(iot.connected_nodes.length);
  
  iot.init_dependant_binds();
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
      
      iot.update_num_connected();
      
      $.ajax({
        url: '/nwk/any/node',
        error: function(e) {
          console.log("error");
        },
        fail: function(e) {
          console.log("fail");
        },
        success: function(d) {
          iot.connected_nodes = d;

          iot.new_nodes = [];

          for (var i = 0; i < iot.networks.length; i++) {
            iot.q++;
            iot.get_new_nodes(iot.networks[i]["_id"]);
          }
        }
      });
    }
  });
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
$.fn.editable.defaults.mode = 'inline';

iot.init_dependant_binds = function() {
  $('button[name="connect"]').on("click", function(event) {
    event.preventDefault();
    $(this).html("Connecting...");
    $(this).prop("disabled", true);
    
    iot.connect_node($(this).attr("id"));
  });
  
  $('button[name="disconnect"]').on("click", function(event) {
    event.preventDefault();
    $(this).html("Disconnecting...");
    $(this).prop("disabled", true);
    
    iot.disconnect_node($(this).attr("id"));
  });

  $('button[name="delete"]').on("click", function(event) {
    event.preventDefault();
    $(this).html("Deleting...");
    //$(this).prop("disabled", true);
    
    iot.delete_node($(this).attr("id"));
  });

  $(".edit_name").editable({
    type:'text',
    title: 'Enter Name',
    success: function(response, value) {
	    var nid = $(this).attr("id");
	    var connected_index = iot.get_connected_node_index_by_id(nid);
	    var new_index = iot.get_new_node_index_by_id(nid);
	    var node;
	    
	    /* normally i'd say we want ==, not a strict typeof ===, but in this
	     * instance we definately want to make sure its the boolean false because
	     * the get_index functions return false if they fail, but the actual index
	     * can be 0, which numerically equivocates to false, so we gotta make sure
	     * and still assign node correctly even if its index 0
	     */
	    if (connected_index !== false)
	      node = iot.connected_nodes[connected_index];
	    
	    if (new_index !== false)
	      node = iot.new_nodes[new_index];
	    
	    if (!node)
	      return false;
	    
	    var old = node["name"];

	    $.ajax({
	      url: '/nwk/' + node["network"] + '/node/' + node["_id"] + '/name/' + value,
	      type: 'put',
	      timeout: 3000,
	      error: function(e) {
	        
	      },
	      fail: function(e) {
	        
	      },
	      success: function(d) {
	        $("#status_message").html('Node: #' + nid + ' "' + old + '" renamed to "' + value + '".');
	        $("#status_message").removeClass("hidden error");
	        $("#status_message").addClass("success");
	      }
	    });
    }
  });

};

iot.init_independant_binds = function() {
  if ($("#main_container").width() >= 960)
    iot.add_menu();
  else
    iot.remove_menu();  
  
  $("#menu_button").on("click", function(event) {
    event.preventDefault();
    iot.toggle_menu();
  });
  
  $('button[name="refresh"]').on("click", function(event) {
    event.preventDefault();
    iot.get_networks();
  });
  
  $("#manage").on("click", function(event) {
    event.preventDefault();
    iot.toggle_manage_targets();
  });
  
  $(window).on("resize", function(event) {
    console.log($("#main_container").width());
    
    if ($("#main_container").width() >= 960)
      iot.add_menu();
    else
      iot.remove_menu();
  });
  
  $('#node_filter').on("change", function(event) {
    switch($(this).val()) {
      case 'new':
        iot.get_networks();
        break;
      case 'connected':
        iot.get_networks();
        break;
      case 'all': /* fall through */
        iot.get_networks();
        break;
      default:
        iot.get_networks();
        break;
    }
  });
  
  
};

iot.set_nav_active = function(){
	var thisPage = window.location.href.substr(window.location.href.lastIndexOf('/')+1);
	$('#navbar').find('a[href="'+thisPage+'"]').find('.nav_element').addClass('active');
};

jQuery(document).ready(function() {
  iot.toggle_manage_targets();
  iot.navbar_groups();
  iot.set_nav_active();
  iot.get_networks();

  iot.init_independant_binds();
  iot.connect_websocket();

});
