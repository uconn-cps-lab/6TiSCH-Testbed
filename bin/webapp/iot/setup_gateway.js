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

var iot = iot || {};
iot.selected_network;

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

iot.set_nav_active = function(){
	var thisPage = window.location.href.substr(window.location.href.lastIndexOf('/')+1);
	$('#navbar').find('a[href="'+thisPage+'"]').find('.nav_element').addClass('active');
};

jQuery(document).ready(function() {
  /* prevent the form from being submitted by pressing enter in a form field */
  $(window).keydown(function(event){
    if(event.keyCode == 13) {
      event.preventDefault();
      return false;
    }
  });
  
  $('input[type="radio"][name="gwMultiple"]').click(function(event) {
    var target = $(event.target);
    
    $('input[type="radio"][name="gwMultiple"]').parent().removeClass("radio_group_selected");
    target.parent().addClass("radio_group_selected");

    if (target.attr("id") == "single_gateway")
      iot.gateway_single();
    else if (target.attr("id") == "multiple_gateway")
      iot.gateway_multiple();
    else
      console.log("error");
    
    $('#api_version option[value="1"]').prop("selected", true);
  });
  
  $('input[type="radio"][name="appOnER"]').click(function(event) {
    var target = $(event.target);
    
    $('input[type="radio"][name="appOnER"]').parent().removeClass("radio_group_selected");
    target.parent().addClass("radio_group_selected");
    
    if (target.attr("id") == "server_local")
      iot.server_local();
    else if (target.attr("id") == "server_cloud")
      iot.server_cloud();
    else
      console.log("error");
    
    $('#api_version option[value="1"]').prop("selected", true);
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
      $("#status_message").html('New configuration accepted and saved.  Continuing to initial node connection.');
      
      $(window).scrollTop($(window).scrollTop());
      
      /* redirect to next step here after short timeout to let people
       * know that their chosen configuration was accepted
       */
      setTimeout(function() {
        window.location.href = '/setup_nodes.html';
      }, 5000);
    }
  });
});