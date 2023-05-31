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

iot.active = { };
iot.active.manage_targets = false;

/* -------------------------------------------------------------------------- */
$.fn.editable.defaults.mode = 'inline';

iot.group_table = function() {
  var buf = '';
      
  buf += '<table>';
  buf += '<tr class="table_header"><th width="30%">';
  buf += 'GROUP NAME';
  buf += '</th><th width="40%">';
  buf += 'DESCRIPTION';
  buf += '</th><th>';
  buf += 'NODES';
  buf += '</th><th>';
  buf += 'ACTIONS';
  buf += '</th></tr>';
  
  for (i = 0; i < iot.groups.length; i++) {
    buf += '<tr id="group_' + iot.groups[i]["name"] + '"><td><span class="fa fa-pencil"></span><div id="' + iot.groups[i]["name"] + '" class="edit_name editable editable-click inline-input">';
    buf += iot.groups[i]["name"];
    buf += '</div></td><td><span class="fa fa-pencil"></span><div id="group_description" name="'+iot.groups[i]["name"]+'" class="edit_description editable editable-click inline-input">';
    buf += decodeURI(iot.groups[i]["description"]);
    buf += '</div></td><td>';
    buf += iot.groups[i]["count"];
    buf += '</td><td>';
    buf += '<button id="' + iot.groups[i]["name"] + '" type="button" name="delete">Delete</button>';
    buf += '</td></tr>';
  }

  if (iot.groups.length < 1)
    buf += '<tr><td><div class="divider"></td><td><div class="divider"></td><td><div class="divider"></td><td><div class="divider"></td></tr>';

  buf += '</table>';
  buf += '<br />';
  buf += '<button id="add_group" type="button" name="add_group">Add</button>';

  $("#group_info").html(buf);
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
      
      iot.group_table();
      
      iot.update_num_connected();
      
      iot.init_dependant_binds();
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

iot.init_dependant_binds = function() {
  $(".edit_name").editable({
    type:'text',
    title: 'Enter Name',
    success: function(response, value) {
    	var old = $(this).attr("id");
	    console.log(old);
	    $.ajax({
	      url: '/group/' + $(this).attr("id") + '/rename/' + value,
	      type: 'PUT',
	      error: function(e) {
	        
	      },
	      fail: function(e) {
	        
	      },
	      success: function(d) {
	        iot.navbar_groups();
	        $("#status_message").html('Group: "' + old + '" renamed to "' + value + '"');
	        $("#status_message").removeClass("hidden");
	      }
	    });
    }
  });
  
  $(".edit_description").editable({
    type:'text',
    title: 'Enter Description',
    success: function(response, value) {

	    var group = $(this).attr("name");

	    $.ajax({
	      url: '/group/' + group + '/desc',
	      type: 'PUT',
	      data: {
	        desc: encodeURI(value)
	      },
	      error: function(e) {
	        
	      },
	      fail: function(e) {
	        
	      },
	      success: function(d) {
	        iot.navbar_groups();
	        $("#status_message").html('Description for group: "' + group + '" changed to "' + decodeURI(value) + '"');
	        $("#status_message").removeClass("hidden");
	      }
	    });
  	}
  });
  
  $('button[name="delete"]').on("click", function(event) {
    event.preventDefault();
    
    var name = $(this).attr("id");
    
    $.ajax({
      url: '/group/' + name,
      type: 'DELETE',
      error: function(e) {
        
      },
      fail: function(e) {
        
      },
      success: function(d) {
        iot.navbar_groups();
        $("#status_message").html('Group: "' + name + '" has been deleted.');
        $("#status_message").removeClass("hidden");
      }
    });
  });
  
  $("#add_group").on("click", function(event) {
    event.preventDefault();
    
    $.ajax({
      url: '/group/NewGroup'+Math.floor((Math.random() * 1000) + 1),
      type: 'PUT',
      error: function(e) {
        
      },
      fail: function(e) {
        
      },
      success: function(d) {
        iot.navbar_groups();
        $("#status_message").html('A new group has been added.');
        $("#status_message").removeClass("hidden");
      }
    });    
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
};

iot.set_nav_active = function(){
	var thisPage = window.location.href.substr(window.location.href.lastIndexOf('/')+1);
	$('#navbar').find('a[href="'+thisPage+'"]').find('.nav_element').addClass('active');
};

jQuery(document).ready(function() {
  iot.toggle_manage_targets();
  iot.navbar_groups();
  iot.set_nav_active();
  iot.init_independant_binds();  
});