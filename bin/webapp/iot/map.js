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

iot.network_map = {};

iot.network_map_bounds;
iot.network_map_markers = new Array();
iot.network_map_marker_selection;

iot.num_nodes = 0;
iot.selected_gateway = 0;

iot.groups = new Array();
iot.networks = new Array();

iot.active = {};

/* -------------------------------------------------------------------------- */

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
      iot.network_map_marker_selection = i;
      $('select[id="gateway"] option[value="' + this.index + '"]').prop("selected", true);
      iot.network_map.panTo(this.position);
      google.maps.event.trigger(iot.network_map, 'resize');
      $("#network_map_modal").modal("hide");
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

iot.grid_ui_map = function() {
  return '<a id="open_network_map_modal" class="open_network_map_modal"></a>';
};

iot.grid_ui_gateways = function() {
  var buf = '';
  var selected = '';

  buf += '<label for="gateway">Gateway&nbsp;</label>';
  buf += '<select id="gateway" name="gateway" class="dropdown">';

  for (var i = 0; i < iot.networks.length; i++) {
    if (i == iot.gateway.index)
      selected = 'selected="selected"';
    
    buf += '<option ' + selected + ' value="' + i + '">' + iot.networks[i].info["netname"] + '</option>';
    
    if (selected.length > 0)
      selected = '';
  }

  buf += '</select>';


  return buf;
};

iot.get_status = function(type, value) {
  switch (type) {
    /* eventually overall status needs to be replaced with a fallthrough block
     * based upon the values gathered from the other sensors
     */
    case 'status':
      if (value > 0) return '<span class="gridicon status_ok"></span>';
	  else return '<span class="gridicon status_inactive"></span>';
    case 'temperature':
      if (value > 0) return '<span class="gridicon temp_ok"></span>';
	  else return '<span class="gridicon temp_inactive"></span>';
    case 'luminosity':
      if (value > 0) return '<span class="gridicon lum_ok"></span>';
	  else return '<span class="gridicon lum_inactive"></span>';
    case 'sound':
      if (value > 0) return '<span class="gridicon sound_ok"></span>';
	  else return '<span class="gridicon sound_inactive"></span>';;
    case 'humidity':
      if (value > 0) return '<span class="gridicon hum_ok"></span>';
	  else return '<span class="gridicon hum_inactive"></span>';
    case 'motion':
      if (value > 0) return '<span class="gridicon mot_ok"></span>';
	  else return '<span class="gridicon mot_inactive"></span>';
    case 'actuator_light':
      if (value > 0) return '<span class="gridicon act_on"></span>';
	  else return '<span class="gridicon act_off"></span>';
    case 'actuator_gpio':
      if (value > 0) return '<span class="gridicon act_on"></span>';
	  else return '<span class="gridicon act_off"></span>';
    default:
      break;
  }
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

  $('#node_list_link').attr('href', 'list.html?gateway=' + iot.gateway.id);
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

iot.load_nodes_tree = function(network) {
/*  var nodeMap = $("#node_map");
  nodeMap.empty();
  var width = nodeMap.width();
  var height = $(window).height() - nodeMap.offset().top - 80; //80 = 60px footer + 20 spacing. bound to change when psds arrive
//  var height = width;
  var link_distance = 150;
  var radius = 20;

  var color = d3.scale.category20();
  var force = d3.layout.force()
          .charge(-120)
		  .gravity(0.05)
		  //.linkStrength(0.1)
		  .friction(0.7) 
          .linkDistance(link_distance)
          .size([width, height]);
  var svg = d3.select("#node_map")
          .append("svg")
          .attr("width", width)
          .attr("height", height)
          .attr("id", "svg-node");

  d3.json("/nwk/" + network + "/node", function(e, d) {
    var data = d;
    var shorts = [];
    var ids = [];
    var status = [];

    shorts.push(1);
    ids.push(1);
    status.push(1);

    $.each(d, function(i, el) {
      iot.randomStatus(el);
      var statusVal = (el.status.rtg_up_pnt[0].outgoingLqi != undefined ? el.status.rtg_up_pnt[0].outgoingLqi : 255);
      shorts.push(el.info.msa);
      ids.push(el._id);
      status.push(statusVal);
    });

    var nodes = [];
    var links = [];

    nodes.push({"name": "root", "group": 1, "index": 0, "id": "root"});

    for (var i = 1; i < shorts.length; i++) {
      nodes.push({
        "name": shorts[i],
        "id": ids[i],
        "group": 2,
        "index": i
      });
    }

    for (var i = 1; i < shorts.length; i++) {
      links.push({
        "source": i,
        "target": shorts.indexOf(d[i - 1].info.rtg_up_pnt[0].addr),
        "value": 1,
        "lqi": status[i]
      });
    }

    force.nodes(nodes)
            .links(links)
            .start();

    var link = svg.selectAll(".link")
            .data(links)
            .enter().append("line")
            .attr("class", function(d) {
              return "link " + iot.getLinkQualityTag(d.lqi);
            });

    node = svg.selectAll(".node")
            .data(nodes)
            .enter().append("circle")
            .attr('id', function(d) {
              //var nodeId = (d.id != undefined ? d.id : 'root');
              //return 'id_' + nodeId;
              return 'id_' + d.id;
            })
            .attr("data-index", function(d) {
              return d.index;
            })
            .attr("class", "node")
            .attr("r", radius)
            .style("fill", function(d) {
              return color(d.group);
            })
            .call(force.drag)
            .attr("stroke", "grey")
            .style("stroke-width", 0);

    node.append("title")
            .text(function(d) {
              return d.name;
            });

    var $flyout = $('#node-flyout'),
		$closeBtn = $('#node-flyout-close'),
		$nodeName = $('#node-flyout-name'),
		$nodeId = $('#node-flyout-id'),
		$parentId = $('#node-flyout-parent-id'),
		
		$sensors = $('#node-flyout-sensors'),
		$status = $sensors.next();
		$lightgpio = $status.next();


    $closeBtn.click(function(e) {
      $flyout.css('display', 'none');
    });

    //node.on("mousedown.drag", null);
    //d3.select("#node_map")

    var flyoutPosition = function(d) {
      var leftAdjust = 40;
      var topAdjust = 60;
      var flyLeft = d.x + $flyout.width() - leftAdjust;
      var flyTop = d.y - topAdjust;
      $flyout.show().css({
        'top': flyTop + 'px',
        'left': flyLeft + 'px'
      });
    };

    var resetFlyout = function() {
      $sensors.html('');
	  $status.html('');
	  $lightgpio.html('');
	  
      $nodeName.text('');
      $nodeId.text('');
      $parentId.text('');
    };

    node.on("click", function(d) {
	  if (d3.event.defaultPrevented) return;
      var $ele = $(this),
              eleIndex = parseInt($ele.attr('data-index')) - 1,
              nodeData = data[eleIndex],
              isRoot = $ele.attr('id') == 'id_root';

      resetFlyout();

      if (isRoot) {
        console.log('don\'t have info for root stuff yet')
      } else if (nodeData != undefined) {
        $nodeName.text(nodeData['_id']);
        $nodeId.text(nodeData.info.dev);
        $parentId.text(nodeData.network);
		
		var tmp = '';
		tmp += iot.get_status('temperature', nodeData['sensor'].temp);
		tmp += iot.get_status('luminosity', nodeData['sensor'].light);
		tmp += iot.get_status('sound', nodeData['sensor'].sound);
		tmp += iot.get_status('humidity', nodeData['sensor'].hum);
		tmp += iot.get_status('motion', nodeData['sensor'].pres);
		$sensors.append(tmp);
		
		tmp = '';
		//tmp += iot.get_status('status', nodeData['status'].pwr_flgs);
		//tmp += '<span>' + Math.round(((Date.now() - (nodeData['timestamp'])) / 1000 / 60)) + 'm ago</span> ';
		tmp += '<span>' + 'Coin Cell </span> ';
		tmp += '<div class="battery"><div class="battery_meter"><div style="left:'+ 10 +'days"></div></div></div>';
    tmp += '\r' + '<span>' + 319.5 + 'days (Estimated for TSCH only)</span>';
		$status.append(tmp);
		
		//tmp = '';
		//tmp += iot.get_status('actuator_light', nodeData['status'].actuator_light);
		//tmp += iot.get_status('actuator_gpio', nodeData['status'].actuator_gpio);
		//$lightgpio.append(tmp);
        
        //$light.addClass(iot.get_status('actuator_light', nodeData['status'].actuator_light));
        //$gpio.addClass(iot.get_status('actuator_gpio', nodeData['status'].actuator_gpio));

        $flyout.show();
        flyoutPosition(d);

      }
    });

    force.on("tick", function() {
      link.attr("x1", function(d) {
        return d.source.x;
      })
              .attr("y1", function(d) {
                return d.source.y;
              })
              .attr("x2", function(d) {
                return d.target.x;
              })
              .attr("y2", function(d) {
                return d.target.y;
              });

      node.attr("cx", function(d) {
        return d.x;
      })
              .attr("cy", function(d) {
                return d.y;
              });
    });
    
//    $("#num_nodes").html(nodes.length);
  });*/
};

iot.getLinkQualityTag = function(lqi) {
  var ret,
          lqiLabels = ['lqi-low', 'lqi-medium', 'lqi-normal'];
  if (lqi < 85)
    ret = lqiLabels[0];
  else if (lqi < 170)
    ret = lqiLabels[1];
  else
    ret = lqiLabels[2];
  return 0;
};

//remove ths function once this is not needed for test's sake
iot.randomStatus = function(el) {
  var tmp;

  //lqi
  el.status.rtg_up_pnt = [];
  tmp = {'outgoingLqi': 255};
  el.status.rtg_up_pnt.push(tmp);

  //status obj
  el.status.pwr_flgs = Math.floor(Math.random() * 5);
  el.status.pwr_lvl = Math.floor(Math.random() * 101);
  el.status.actuator_light = 1;
  el.status.actuator_gpio = 0;

  //sensor obj
  el.sensor.temp = Math.floor(Math.random() * 5);
  el.sensor.light = Math.floor(Math.random() * 5);
  el.sensor.sound = Math.floor(Math.random() * 5);
  el.sensor.hum = Math.floor(Math.random() * 5);
  el.sensor.pres = Math.floor(Math.random() * 5);
};

iot.url_parse_gateway = function() {
  var result = false;
  var temp = [];

  window.location.search.substr(1).split("&").forEach(function(item) {
    temp = item.split("=");

    if (temp[0] === "gateway")
      result = temp[1];
  });

  return result;
};

iot.init_dependant_binds = function() {
  $('#gateway').on("change", function() {
    iot.gateway.index = $(this).val();
    iot.gateway.id = iot.networks[iot.gateway.index]._id;
    
    $('#node_list_link').attr('href', 'list.html?gateway=' + iot.gateway.id);

    iot.load_nodes_tree(iot.gateway.id);
  });
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
    //going to delay the redraw after the user is finished redrawing
    clearTimeout(iot.redrawTimer);
    iot.redrawTimer = setTimeout(function() {
      iot.load_nodes_tree(iot.gateway.id);
    }, 500);
    
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

  $.ajax({
    url: '/nwk',
    error: function(e) {
      console.log("error");
    },
    fail: function(e) {
      console.log("fail");
    },
    success: function(d) {
      iot.networks = d;

      var gateway = iot.url_parse_gateway();
      var gatewayIndex;

      if (!gateway) {
        gatewayIndex = 0;
        gateway = d[0]._id;
      } else {
        for (var i = 0; i < d.length; i++) {
          if (gateway == d[i]._id) {
            gatewayIndex = i;
            break;
          }
        }
      }

      iot.gateway = {'id': gateway, 'index': gatewayIndex}

      iot.grid_ui();
      iot.init_dependant_binds();
      iot.load_network_map();
      iot.load_nodes_tree(gateway);

      $("#open_network_map_modal").on("click", function() {
        $("#network_map_modal").modal("toggle");
        google.maps.event.trigger(iot.network_map, 'resize');
      });
    }
  });
  iot.init_independant_binds();
});
