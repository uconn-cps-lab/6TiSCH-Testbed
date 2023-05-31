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

iot.groups = new Array();

iot.selected_network;

iot.config = { };
iot.config.default = { };

iot.config.default.name = '';
iot.config.default.gps = '';
iot.config.default.local = '';
iot.config.default.gateway = '';
iot.config.default.url = '';
iot.config.default.proxy = '';
iot.config.default.port = '';
iot.config.default.keys = '';
iot.config.default.api = '';

iot.config.name = '';
iot.config.gps = '';
iot.config.local = '';
iot.config.gateway = '';
iot.config.url = '';
iot.config.proxy = '';
iot.config.port = '';
iot.config.keys = '';
iot.config.api = '';

iot.active = {};
iot.active.manage_targets = false;

/* -------------------------------------------------------------------------- */

iot.gateway_single = function() {
  $("#master_gateway_url").html('');
};

iot.gateway_multiple = function() {
  var buf = '';

  buf += '<div class="row">';

  buf += '<div class="col-xs-3 text-right">';
  buf += '<label for="gateway_url">Master Gateway URL</label>';
  buf += '</div>';

  buf += '<div class="col-xs-6">';
  buf += '<input type="text" id="gateway_url" name="nwk" />';
  buf += '</div>';

  buf += '<div class="col-xs-3">';
  buf += '<div class="config_details">';
  buf += 'Leave blank if you\'d like this Gateway to be the Master.';
  buf += '</div>';
  buf += '</div>';

  buf += '</div>';

  $("#master_gateway_url").html(buf);
};

iot.server_local = function() {
  var buf = '';
      
  buf += '<div class="row">'; /* ++server url */

  buf += '<div class="col-xs-3 text-right">';
  buf += '<label for="server_url">Server URL</label>';
  buf += '</div>';

  buf += '<div class="col-xs-6">';
  buf += '<input type="text" id="server_url" name="server" />';
  buf += '</div>';

  buf += '<div class="col-xs-3">';
  buf += '<div class="config_details">';
  buf += 'This will be used in the URL to access the gateway.';
  buf += '</div>';
  buf += '</div>';

  buf += '</div>'; /* --server url */

  buf += '<div class="row">'; /* ++api version */

  buf += '<div class="col-xs-3 text-right">';
  buf += '<label for="api_version">API Version</label>';
  buf += '</div>';

  buf += '<div class="col-xs-6 form_control">';
  buf += '<select id="api_version" name="api_ver" class="dropdown">';
  buf += '<option value="1" selected="selected">1</option>';
  buf += '</select>';
  buf += '</div>';

  buf += '<div class="col-xs-3">';
  buf += '<div class="config_details">';
  buf += 'You can use different API versions for testing nodes.';
  buf += '</div>';
  buf += '</div>';

  buf += '</div>'; /* --api version */

  $("#server_settings").html(buf);
};

iot.server_cloud = function() {
  var buf = '';
      
  buf += '<div class="row">'; /* ++remote host */

  buf += '<div class="col-xs-3 text-right">';
  buf += '<label for="server_url">Remote Server Hostname</label>';
  buf += '</div>';

  buf += '<div class="col-xs-6">';
  buf += '<input type="text" id="server_url" name="server" />';
  buf += '</div>';

  buf += '<div class="col-xs-3">';
  buf += '<div class="config_details">';
  buf += 'Enter the URL of your remote server that will accept this gateway.';
  buf += '</div>';
  buf += '</div>';

  buf += '</div>'; /* --remote host */

  buf += '<div class="row">'; /* ++proxy */

  buf += '<div class="col-xs-3 text-right">';
  buf += '<label for="server_proxy">Proxy</label>';
  buf += '</div>';

  buf += '<div class="col-xs-6">';
  buf += '<input type="text" id="server_proxy" name="proxy" />';
  buf += '</div>';

  buf += '<div class="col-xs-3">';
  buf += '<div class="config_details">';
  buf += '';
  buf += '</div>';
  buf += '</div>';

  buf += '</div>'; /* --proxy */

  buf += '<div class="row">'; /* ++port */

  buf += '<div class="col-xs-3 text-right">';
  buf += '<label for="server_port">Port</label>';
  buf += '</div>';

  buf += '<div class="col-xs-6">';
  buf += '<input type="text" id="server_port" name="port" />';
  buf += '</div>';

  buf += '<div class="col-xs-3">';
  buf += '<div class="config_details">';
  buf += '';
  buf += '</div>';
  buf += '</div>';

  buf += '</div>'; /* --port */

  buf += '<div class="row">'; /* ++api keys */

  buf += '<div class="col-xs-3 text-right">';
  buf += '<label for="api_keys">API Key(s)</label>';
  buf += '</div>';

  buf += '<div class="col-xs-6">';
  buf += '<input type="text" id="api_keys" name="keys" />';
  buf += '</div>';

  buf += '<div class="col-xs-3">';
  buf += '<div class="config_details">';
  buf += 'Enter the API Keys provided by the remote server.';
  buf += '</div>';
  buf += '</div>';

  buf += '</div>'; /* --proxy */

  buf += '<div class="row">'; /* ++api version */

  buf += '<div class="col-xs-3 text-right">';
  buf += '<label for="api_version">API Version</label>';
  buf += '</div>';

  buf += '<div class="col-xs-6 form_control">';
  buf += '<select id="api_version" name="api_ver" class="dropdown">';
  buf += '<option value="1" selected="selected">1</option>';
  buf += '</select>';
  buf += '</div>';

  buf += '<div class="col-xs-3">';
  buf += '<div class="config_details">';
  buf += 'You can use different API versions for testing nodes.';
  buf += '</div>';
  buf += '</div>';

  buf += '</div>'; /* --api version */

  $("#server_settings").html(buf);
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
      
      iot.update_num_connected();
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

iot.load_gateway_config = function(config) {
  if (config.name.length > 0)
    $("#gateway_name").val(config.name);
  else
    $("#gateway_name").val(config.default.name);
  
  if (config.gps.length > 0)
    $("#gateway_gps").val(config.gps);
  else
    $("#gateway_gps").val(config.default.gps);

  if (config.gateway.length > 0)
    $("#gateway_url").val(config.gateway);
  else
    $("#gateway_url").val(config.default.gateway);

  if (config.url.length > 0)
    $("#server_url").val(config.url);
  else
    $("#server_url").val(config.default.url);
  
  if (config.proxy.length > 0)
    $("#server_proxy").val(config.proxy);
  else
    $("#server_proxy").val(config.default.proxy);

  if (config.port.length > 0)
    $("#server_port").val(config.port);
  else
    $("#server_port").val(config.default.port);
    
  if (config.keys.length > 0)
    $("#api_keys").val(config.keys);
  else
    $("#api_keys").val(config.default.keys);
  
  $('#api_version option[value="1"]').prop("selected", true);
  /* for now just set version to 1 */
};

iot.get_gateway_config = function() {
  $.ajax({
    url: '/config/gw',
    error: function(e) {
      console.log("error");
    },
    fail: function(e) {
      console.log("fail");
    },
    success: function(d) {
      iot.config.default = d;
      
      /* reset our cache'd variables so they don't override */
      iot.config.name = '';
      iot.config.gps = '';
      iot.config.local = '';
      iot.config.gateway = '';
      iot.config.url = '';
      iot.config.proxy = '';
      iot.config.port = '';
      iot.config.keys = '';
      iot.config.api = '';
      
      /* set our new defaults to compare our cache to */
      iot.config.default = { };
      iot.config.default.name = d.nwkinfo["netname"];
      iot.config.default.gps = d.nwkinfo["gps"];
      iot.config.default.local = d["appOnER"];
      iot.config.default.gateway = d.conninfo["nwk"];
      iot.config.default.url = d.conninfo["server"];
      iot.config.default.proxy = d.conninfo["proxy"];
      iot.config.default.port = d.conninfo["port"];
      iot.config.default.keys = d.conninfo["keys"];
      iot.config.default.api = d.conninfo["api_ver"];
      
      iot.load_gateway_config(iot.config);
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

iot.set_form_cache = function() {
  if ($('#gateway_name').val() != iot.config.default.name)
    iot.config.name = $('#gateway_name').val() ? $('#gateway_name').val() : '';
  
  if ($('#gateway_gps').val() != iot.config.default.gps)
    iot.config.gps = $('#gateway_gps').val() ? $('#gateway_gps').val() : '';
 
  if ($('#gateway_url').val() != iot.config.default.gateway)
    iot.config.gateway = $('#gateway_url').val() ? $('#gateway_url').val() : '';

  if ($('#server_url').val() != iot.config.default.url)
    iot.config.url = $("#server_url").val() ? $("#server_url").val() : '';
  
  if ($('#server_proxy').val() != iot.config.default.proxy)
    iot.config.proxy = $('#server_proxy').val() ? $('#server_proxy').val() : '';
  
  if ($('#server_port').val() != iot.config.default.port)
    iot.config.port = $('#server_port').val() ? $('#server_port').val() : '';
  
  if ($('#api_keys').val() != iot.config.default.keys)
    iot.config.keys = $('#api_keys').val() ? $('#api_keys').val() : '';
};

iot.set_nav_active = function() {
	var thisPage = window.location.href.substr(window.location.href.lastIndexOf('/')+1);
	$('#navbar').find('a[href="'+thisPage+'"]').find('.nav_element').addClass('active');
};

jQuery(document).ready(function() {
  iot.toggle_manage_targets();
  iot.navbar_groups();
  iot.set_nav_active();
  iot.get_gateway_config();
  
  /* prevent the form from being submitted by pressing enter in a form field */
  $(window).keydown(function(event){
    if(event.keyCode == 13) {
      event.preventDefault();
      return false;
    }
  });
  
  if ($("#main_container").width() >= 960)
    iot.add_menu();
  else
    iot.remove_menu();  
  
  $("#menu_button").on("click", function(event) {
    event.preventDefault();
    iot.toggle_menu();
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
  
  $('input[type="radio"][name="gwMultiple"]').click(function(event) {
    var target = $(event.target);
    
    iot.set_form_cache();
    
    $('input[type="radio"][name="gwMultiple"]').parent().removeClass("radio_group_selected");
    target.parent().addClass("radio_group_selected");

    if (target.attr("id") == "single_gateway")
      iot.gateway_single();
    else if (target.attr("id") == "multiple_gateway")
      iot.gateway_multiple();
    else
      console.log("error");
    
    iot.load_gateway_config(iot.config);
  });
  
  $('input[type="radio"][name="appOnER"]').click(function(event) {
    var target = $(event.target);
    
    iot.set_form_cache();
    
    $('input[type="radio"][name="appOnER"]').parent().removeClass("radio_group_selected");
    target.parent().addClass("radio_group_selected");
    
    if (target.attr("id") == "server_local")
      iot.server_local();
    else if (target.attr("id") == "server_cloud")
      iot.server_cloud();
    else
      console.log("error");
    
    iot.load_gateway_config(iot.config);
  });
  
  $('#reset_form').on("click", function(event) {
    iot.get_gateway_config();
  });
  
  $("#setup_gateway").ajaxForm({
    error: function() {
      $("#status_message").removeClass("hidden success");
      $("#status_message").addClass("error");
      $("#status_message").html('New configuration not accepted.');
      
      $(window).scrollTop($(window).scrollTop());
    },
    success: function() {
      $("#status_message").removeClass("hidden error");
      $("#status_message").addClass("success");
      $("#status_message").html('New configuration saved.');
      
      $(window).scrollTop($(window).scrollTop());
      
      iot.get_gateway_config(); /* the data's changed, so let's update it */
    }
  });
});