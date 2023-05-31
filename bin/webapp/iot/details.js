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
iot.node;
iot.map;
iot.nodes;
iot.groups = new Array();

iot.selected_node;

iot.sensor_chart;
iot.sensor_canvas;
iot.power_chart;
iot.power_canvas;
iot.actuator_chart;
iot.actuator_canvas;
iot.power_details=false;

iot.sensor_series = { 
"temp":new TimeSeries(),
"rhum":new TimeSeries(),
"lux":new TimeSeries(),
"press":new TimeSeries(),
"accelx":new TimeSeries(),
"accely":new TimeSeries(),
"accelz":new TimeSeries(),
"channel":new TimeSeries()
};


iot.power_series = {
"bat":new TimeSeries(),
"eh":new TimeSeries(),
"active":new TimeSeries(),
"sleep":new TimeSeries(),
"total":new TimeSeries(),
"cc2650_active":new TimeSeries(),
"cc2650_sleep":new TimeSeries(),
"rf_tx":new TimeSeries(),
"rf_rx":new TimeSeries(),
"gpsen_active":new TimeSeries(),
"gpsen_sleep":new TimeSeries(),
"msp432_active":new TimeSeries(),
"msp432_sleep":new TimeSeries(),
"others":new TimeSeries(),
};

iot.actuator_series = { };
iot.actuator_series.light;

iot.socket;

iot.active = {};
iot.active.tab = 'sensors';
iot.active.sensors = { };

iot.active.manage_targets = false;

iot.canvas_width;

/* -------------------------------------------------------------------------- */

iot.url_parse_node = function() {
  var result = false;
  var temp = [];

  window.location.search.substr(1).split("&").forEach(function(item) {
    temp = item.split("=");

    if (temp[0] === "id")
      result = temp[1];
  });

  return result;
};

iot.change_parent = function() {
  var new_parent = prompt("Please enter new parent short address");
  if(new_parent == null)return;
  if(isNaN(new_parent)){
    alert("Invalid parent short address");
    return;
  }else{
    $.ajax({
      url: '/nodes/'+iot.selected_node+'/diagnosis/'+new_parent,
      type: 'put'
    })
    alert("Command sent");
  }
}
iot.get_diagnosis_info = function() {
  $("#diagnosis_info").text("Querying...");
  var t1=new Date().getTime();
  $.ajax({
    url: '/nodes/'+iot.selected_node+'/diagnosis/',
    error: function(e) {
      $("#diagnosis_info").text("Error");
    },
    fail: function(e) {
      $("#diagnosis_info").text("Failed");
    },
    success: function(d) {
      var t2=new Date().getTime();
      s = d.split(",")
      table1 = ""
      table1+= "<table>";
      table1+= "<tr><td>PER</td><td># Parent Change</td><td># Sync Lost</td><td>Avg. Drift </td><td>Max Drift</td><td>Avg. RSSI</td><td># Candidate Parent</td><td>Beacon Lost Rate</td><td>Association Time</td></tr>";
      table1+= "<tr><td>"+s[0]+"%</td><td>"+s[1]+"</td><td>"+s[2]+"</td><td>"+s[3]+" us</td><td>"+s[4]+" us</td><td>"+s[5]+" dbm</td><td>"+s[6]+"</td><td>"+s[7]+"%</td><td>"+s[9]+" s</td></tr>";
      table1+= "</table>"; 
      table1+= "<table>";
      table1+= "<tr><td>ShortAddr</td><td>JoinPriority</td><td>RSSI</td><td>LQI</td></tr>";
      for(var i=10;i<s.length-1;++i){
        w=s[i].split(" ");
        table1+= "<tr><td>"+w[0]+"</td><td>"+w[1]+"</td><td>"+w[2]+"</td><td>"+w[3]+"</td></tr>";
      }
      table1+= "</table>"; 
      table1+= "<table>";
      table1+= "<tr><td>Round Trip Time</td><td>Application PER</td></tr>";
      table1+= "<tr><td>"+(t2-t1)/1000+"s</td><td>"+(iot.node_details.app_per*100).toFixed(2)+"%</td></tr>";
      table1+= "</table>"; 
      $("#diagnosis_info").html(table1);
    }
  })
}

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

iot.init_sensor_graph = function() {
  iot.sensor_chart = new SmoothieChart({
    millisPerPixel: 48,
    interpolation: 'bezier',
    grid: {
/*      strokeStyle: 'rgba(192, 192, 192, 0.7)',
      sharpLines: true,
      verticalSections: 5,
      borderVisible: false*/
      strokeStyle:'transparent'
    },
    labels: {
      fontSize: 11
    }
  });

  iot.sensor_canvas = document.getElementById("sensor_canvas");
                       
  iot.sensor_chart.streamTo(iot.sensor_canvas, 500);
  iot.canvas_width = $("#sensor_content").width();
  iot.sensor_canvas["width"] = iot.canvas_width;
};

iot.init_power_graph = function() {
  iot.power_chart = new SmoothieChart({
    millisPerPixel: 48,
    interpolation: 'linear',
    grid: {
/*      strokeStyle: 'rgba(192, 192, 192, 0.7)',
      sharpLines: true,
      verticalSections: 5,
      borderVisible: false*/
      strokeStyle:'transparent'
    },
    labels: {
      fontSize: 11
    }
  });

  iot.power_canvas = document.getElementById("power_canvas");

  iot.power_chart.streamTo(iot.power_canvas, 500);
  iot.power_canvas["width"] = iot.canvas_width;
};

iot.load_graphs = function() {
  iot.socket = new WebSocket("ws://"+location.hostname+":5000/");

  iot.socket.onmessage = function(event) {
    var data = JSON.parse(event.data);
    console.log(data);

    function NA(){
      $("#sensors_status").html(iot.get_status('status',-3))
      $("#actuator_status").html(iot.get_status('status',-3))

      $("#sensors_temperature").html('N.A.');
      $("#sensors_humidity").html('N.A.');
      $("#sensors_luminosity").html('N.A.');
      $("#sensors_pressure").html('N.A.');
      $("#sensors_channel").html('N.A.');
      $("#sensors_accelerometer").html('N.A.');

      $("#actuator_light").html(iot.get_status('actuator_light', -1));

      for(var name in iot.power_series){
        $("#power_"+name).html('N.A.');
      }
    }

    if (iot.selected_node == data["_id"]) {
      if (data.sensors==null){
        NA();
      }else{
        data.sensors.active=
          data.sensors.cc2650_active+
          data.sensors.rf_tx+
          data.sensors.rf_rx+
          data.sensors.msp432_active+
          data.sensors.gpsen_active+
          data.sensors.others;
        data.sensors.sleep=
          data.sensors.cc2650_sleep+
          data.sensors.msp432_sleep+
          data.sensors.gpsen_sleep+
          data.sensors.others;
        data.sensors.total=data.sensors.active+data.sensors.sleep;
        var time=new Date().getTime();

        iot.node_details.app_per = data.app_per;
        $("#sensors_status").html(iot.get_status('status',0))
        $("#actuator_status").html(iot.get_status('status',0))

        for(var name in iot.sensor_series){
          iot.sensor_series[name].append(time, data.sensors[name]);
        }
        $("#sensors_temperature").html(data.sensors.temp + ' (&deg;C)');
        $("#sensors_luminosity").html(data.sensors.lux + ' (lux)');
        $("#sensors_pressure").html(data.sensors.press + ' (hPa)');
        $("#sensors_humidity").html(data.sensors.rhum + ' (%)');
        $("#sensors_accelerometer").html(data.sensors.accelx + ' - ' + data.sensors.accely + ' - ' + data.sensors.accelz + ' (g)');

        for(var name in iot.power_series){
          iot.power_series[name].append(time, data.sensors[name]);
          if(name=="bat"){
            $("#power_bat").html(data.sensors.bat + ' (V)');
          }
          else if (name=="eh")
          {
            $("#power_eh").html(data.sensors.eh1 + '/'+data.sensors.eh +'(%)');
          }
          else{
            $("#power_"+name).html(data.sensors[name].toFixed(2) + ' (uA)');
          }
        }
        
        $("#sensors_channel").html(data.sensors.channel);
        iot.node_details.sensors["actuator_light"] = data.sensors.led;
        $("#actuator_light").html(iot.get_status('actuator_light', data.sensors.led) + (data.sensors.led == 1 ? ' ON' : ' OFF'));
      }
    }
  };
};

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
    case 'actuator_light':
      switch (value) {
        case 1:
          return "<span class=\"fa fa-toggle-on status_ok\"></span>";
        case 0:
          return "<span class=\"fa fa-toggle-off status_inactive\"></span>";
        case -1:
          return "<span class=\"fa fa-toggle-off  status_warn\"></span>";
        case -2:
          return "<span class=\"fa fa-toggle-off  status_error\"></span>";
        default:
          return "<span class=\"fa fa-toggle-off  status_inactive\"></span>";
      }
    default:
      break;
  }
};

iot.sensor_header = function() {
  var buf = '';
  
  buf += '<table>';
  buf += '<tr>';
  buf += '<td>STATUS</td>';
  buf += '<td id="sensors_heading_temperature" class="graph_button">TEMPERATURE</td>';
  buf += '<td id="sensors_heading_humidity" class="graph_button">HUMIDITY</td>';
  buf += '<td id="sensors_heading_luminosity" class="graph_button">LUMINOSITY</td>';
  buf += '<td id="sensors_heading_pressure" class="graph_button">PRESSURE</td>';
  buf += '<td id="sensors_heading_accelerometer" class="graph_button">ACCELEROMETER</td>';
  buf += '<td id="sensors_heading_channel" class="graph_button">CHANNEL</td>';
  buf += '</tr>';
  buf += '<tr>';
  buf += '<td id="sensors_status">' + iot.get_status('status', 1) + '</td>';
  buf += '<td id="sensors_temperature" class="graph_button"> N.A. ' + '</td>';
  buf += '<td id="sensors_humidity" class="graph_button"> N.A. ' + '</td>';
  buf += '<td id="sensors_luminosity" class="graph_button"> N.A. ' + '</td>';
  buf += '<td id="sensors_pressure" class="graph_button"> N.A. ' + '</td>';
  buf += '<td id="sensors_accelerometer" class="graph_button"> N.A. ' + '</td>';
  buf += '<td id="sensors_channel" class="graph_button"> N.A. </td>';
  buf += '</tr>';
  buf += '</table>';
  
  return buf;
};

iot.power_header = function() {
  var buf = '';
  
  buf += '<table>';
  buf += '<tr style="border-right: 1px solid">';

  buf += '<td id="power_details">[Details]</td>';

  buf += '<td id="power_head_active" class="graph_button"><table><tr><td>Active</td></tr><tr><td id="power_active"> N.A. </td></tr></table></td>';
  buf += '<td id="power_head_sleep" class="graph_button"><table><tr><td>Sleep</td></tr><tr><td id="power_sleep"> N.A. </td></tr></table></td>';
  buf += '<td id="power_head_total" class="graph_button"><table><tr><td>Total</td></tr><tr><td id="power_total"> N.A. </td></tr></table></td>';
  buf += '<td id="power_head_bat" class="graph_button"><table><tr><td>Battery</td></tr><tr><td id="power_bat"> N.A. </td></tr></table></td>';
  buf += '<td id="power_head_eh" class="graph_button"><table><tr><td>Energy Harvesting</td></tr><tr><td id="power_eh"> N.A. </td></tr></table></td>';

  buf += '</tr><tr class="power_table" style="border-top: 1px solid; border-right:1px solid;">';

  buf += '<td>Wireless</td>';

  buf += '<td id="power_head_cc2650active" class="graph_button"><table><tr><td>Active</td></tr><tr><td id="power_cc2650_active"> N.A. </td></tr></table></td>';
  buf += '<td id="power_head_cc2650sleep" class="graph_button"><table><tr><td>Sleep</td></tr><tr><td id="power_cc2650_sleep"> N.A. </td></tr></table></td>';
  buf += '<td id="power_head_rftx" class="graph_button"><table><tr><td>TX</td></tr><tr><td id="power_rf_tx"> N.A. </td></tr></table></td>';
  buf += '<td id="power_head_rfrx" class="graph_button"><table><tr><td>RX</td></tr><tr><td id="power_rf_rx"> N.A. </td></tr></table></td>';
  buf += '<td></td>';

  buf += '</tr><tr class="power_table" style="border-top: 1px solid; border-right:1px solid;">';

  buf += '<td>Sensors</td>';

  buf += '<td id="power_head_gpsenactive" class="graph_button"><table><tr><td>GPSen Active</td></tr><tr><td id="power_gpsen_active"> N.A. </td></tr></table></td>';
  buf += '<td id="power_head_gpsensleep" class="graph_button"><table><tr><td>GPSen Sleep</td></tr><tr><td id="power_gpsen_sleep"> N.A. </td></tr></table></td>';
  buf += '<td></td><td></td><td></td>';

  buf += '</tr><tr class="power_table" style="border-top: 1px solid; border-right:1px solid;">';

  buf += '<td>MCU</td>';
  buf += '<td id="power_head_msp432active" class="graph_button"><table><tr><td>Active</td></tr><tr><td id="power_msp432_active"> N.A. </td></tr></table></td>';
  buf += '<td id="power_head_msp432sleep" class="graph_button"><table><tr><td>Sleep</td></tr><tr><td id="power_msp432_sleep"> N.A. </td></tr></table></td>';
  buf += '<td></td><td></td><td></td>';
  buf += '</tr><tr class="power_table" style="border-top: 1px solid; border-right:1px solid;">';
  buf += '<td>Others</td>';
  buf += '<td id="power_head_others" class="graph_button"><table><tr><td>PM + DAC</td></tr><tr><td id="power_others"> N.A. </td></tr></table></td>';
  buf += '<td></td><td></td><td></td><td></td>';
  buf += '</tr>';
  buf += '</table>';
  
  return buf;
};

iot.actuator_header = function() {
  var buf = '';
  
  buf += '<table>';
  buf += '<tr>';
  buf += '<td>STATUS</td>';
  buf += '<td class="actuator_light">LIGHT</td>';
  buf += '</tr>';
  buf += '<tr>';
  buf += '<td id="actuator_status">' + iot.get_status('status', 1) + '</td>';
  buf += '<td class="actuator_light" id="actuator_light">' + iot.get_status('actuator_light', 0) + '' + '</td>';
  buf += '</tr>';
  buf += '</table>';
  
  return buf;
};

iot.sensor_controls = function() {
  var buf = '';
  
  buf += '<div id="sensor_ui_text" class="col-xs-3">';
  buf += '</div>';
  buf += '<div class="col-xs-3">';
  buf += '</div>';
  return buf;
};

iot.power_controls = function() {
  var buf = '';
  
  buf += '<div id="power_ui_text" class="col-xs-3">';
  buf += '</div>';
  buf += '<div class="col-xs-3">';
  buf += '</div>';
  return buf;
};

iot.actuator_controls = function() {
  var buf = '';
  
  buf += '<div id="actuator_ui_text" class="col-xs-3">';
  buf += '</div>';
  buf += '<div class="col-xs-3">';
  buf += '</div>';
  return buf;
};

iot.load_tabs = function() {
  $("#sensor_header").html(iot.sensor_header());
  $("#sensor_controls").html(iot.sensor_controls());
  
  $("#power_header").html(iot.power_header());
  $("#power_controls").html(iot.power_controls());
  
  $("#actuator_header").html(iot.actuator_header());
  $("#actuator_controls").html(iot.actuator_controls());
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

iot.bind_actuator_buttons = function () {
  $('.actuator_light').click(function(event) {
    console.log('/nodes/'+iot.selected_node+'/led/' + (1-iot.node_details.sensors["actuator_light"]))
    $.ajax({
      url: '/nodes/'+iot.selected_node+'/led/' + (1-iot.node_details.sensors["actuator_light"]),
      type: 'put',
    });
  })
}

iot.bind_graph_buttons = function () {
  	$('.graph_button').click(function(event) {
		switch(iot.active.sensors) {
			case 'temperature':
          iot.sensor_chart.removeTimeSeries(iot.sensor_series.temp);
          break;
      case 'humidity':
          iot.sensor_chart.removeTimeSeries(iot.sensor_series.rhum);
          break;
      case 'luminosity':
          iot.sensor_chart.removeTimeSeries(iot.sensor_series.lux);
          break;
      case 'pressure':
          iot.sensor_chart.removeTimeSeries(iot.sensor_series.press);
          break;
      case 'accelerometer':
          iot.sensor_chart.removeTimeSeries(iot.sensor_series.accelx);
          iot.sensor_chart.removeTimeSeries(iot.sensor_series.accely);
          iot.sensor_chart.removeTimeSeries(iot.sensor_series.accelz);
          break;
      case 'channel':
          iot.sensor_chart.removeTimeSeries(iot.sensor_series.channel);
          break;
      case 'bat':
          iot.power_chart.removeTimeSeries(iot.power_series.bat);
          break;
      case 'eh':
          iot.power_chart.removeTimeSeries(iot.power_series.eh);
          break;
      case 'active':
          iot.power_chart.removeTimeSeries(iot.power_series.active);
          break;
      case 'sleep':
          iot.power_chart.removeTimeSeries(iot.power_series.sleep);
          break;
      case 'total':
          iot.power_chart.removeTimeSeries(iot.power_series.total);
          break;
      case 'cc2650active':
          iot.power_chart.removeTimeSeries(iot.power_series.cc2650_active);
          break;
      case 'cc2650sleep':
          iot.power_chart.removeTimeSeries(iot.power_series.cc2650_sleep);
          break;
      case 'rftx':
          iot.power_chart.removeTimeSeries(iot.power_series.rf_tx);
          break;
      case 'rfrx':
          iot.power_chart.removeTimeSeries(iot.power_series.rf_rx);
          break;
      case 'gpsenactive':
          iot.power_chart.removeTimeSeries(iot.power_series.gpsen_active);
          break;
      case 'gpsensleep':
          iot.power_chart.removeTimeSeries(iot.power_series.gpsen_sleep);
          break;
      case 'msp432active':
          iot.power_chart.removeTimeSeries(iot.power_series.msp432_active);
          break;
      case 'msp432sleep':
          iot.power_chart.removeTimeSeries(iot.power_series.msp432_sleep);
          break;
      case 'others':
          iot.power_chart.removeTimeSeries(iot.power_series.others);
          break;
      default:
        break;
		}

		var split = $(this).attr('id').split('_');
		iot.active.sensors = split[split.length-1];

		console.log(iot.active.sensors);
		switch(iot.active.sensors) {
			case 'temperature':
        iot.sensor_chart.addTimeSeries(iot.sensor_series.temp, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'humidity':
        iot.sensor_chart.addTimeSeries(iot.sensor_series.rhum, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'luminosity':
        iot.sensor_chart.addTimeSeries(iot.sensor_series.lux, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'pressure':
        iot.sensor_chart.addTimeSeries(iot.sensor_series.press, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'accelerometer':
        iot.sensor_chart.addTimeSeries(iot.sensor_series.accelx, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        iot.sensor_chart.addTimeSeries(iot.sensor_series.accely, { lineWidth: 1.5, strokeStyle: '#ff0000' });
        iot.sensor_chart.addTimeSeries(iot.sensor_series.accelz, { lineWidth: 1.5, strokeStyle: '#0000ff' });
        break;
      case 'channel':
        iot.sensor_chart.addTimeSeries(iot.sensor_series.channel, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;

      case 'bat':
        iot.power_chart.addTimeSeries(iot.power_series.bat, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'eh':
        iot.power_chart.addTimeSeries(iot.power_series.eh, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'active':
        iot.power_chart.addTimeSeries(iot.power_series.active, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'sleep':
        iot.power_chart.addTimeSeries(iot.power_series.sleep, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'total':
        iot.power_chart.addTimeSeries(iot.power_series.total, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'cc2650active':
        iot.power_chart.addTimeSeries(iot.power_series.cc2650_active, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'cc2650sleep':
        iot.power_chart.addTimeSeries(iot.power_series.cc2650_sleep, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'rftx':
        iot.power_chart.addTimeSeries(iot.power_series.rf_tx, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'rfrx':
        iot.power_chart.addTimeSeries(iot.power_series.rf_rx, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'gpsenactive':
        iot.power_chart.addTimeSeries(iot.power_series.gpsen_active, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'gpsensleep':
        iot.power_chart.addTimeSeries(iot.power_series.gpsen_sleep, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'msp432active':
        iot.power_chart.addTimeSeries(iot.power_series.msp432_active, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'msp432sleep':
        iot.power_chart.addTimeSeries(iot.power_series.msp432_sleep, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      case 'others':
        iot.power_chart.addTimeSeries(iot.power_series.others, { lineWidth: 1.5, strokeStyle: '#00ff00' });
        break;
      default:
        break;
		}
	});
};

iot.add_group_dropdown = function() {
  var buf = '';
  
  buf += '<form id="form_add_to_group" action="#" method="POST">';
  buf += '<div class="dropdown">';
  buf += '<a data-toggle="dropdown" href="#">Add to Group<span class="caret"></span></a>';
  buf += '<ul class="dropdown-menu" role="menu" aria-labelledby="add_to_groups">';

  for (var i = 0; i < iot.groups.length; i++) {
    buf += '<li class="add_to_group" value="' + iot.groups[i]["name"] + '">' + iot.groups[i]["name"] + '</li>';
  }

  buf += '<li id="manage_groups" value="manage_groups">Manage Groups</li>';

  buf += '</ul>';
  buf += '</div>';
  buf += '</form>';
  
  return buf;
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
      $("#add_to_group").html(iot.add_group_dropdown());
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

iot.init_dependant_binds = function() {
  $(".add_to_group").on("click", function(event) {
    event.preventDefault();
    event.stopPropagation();
    
    var group = $(this).attr('value');
    var eui_list = [];
    
    eui_list.push(iot.node_details.info["mla"]);
    
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
        $("#status_message").html('This node has been added to group "' + group + '".');
        $("#status_message").removeClass("hidden");
      }
    });
  });
  
  $("#manage_groups").on("click", function(event) {
    event.preventDefault();
    
    window.location.href = 'manage_groups.html';
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
  iot.init_independant_binds();
  
  var nid = iot.url_parse_node();
  
  iot.selected_node = nid;
  
  $.ajax({
    url: '/nodes/'+nid,
    error: function(e) {
      console.log("error");
    },
    fail: function(e) {
      console.log("fail");
    },
    success: function(d) {
      iot.update_num_connected();
      console.log(iot.selected_node);
      iot.node_details = d;
      iot.node_details.sensors={actuator_light:0};
        
      $("#node_name").html(iot.node_details._id);
      if(iot.node_details.address==null)
        iot.node_details.address="Unknown"
      if(iot.node_details.parent==null)
        iot.node_details.parent="Unknown"
      if(iot.node_details.eui64==null)
        iot.node_details.eui64="Unknown"
      if(iot.node_details.meta==null)
        iot.node_details.meta={}
      if(iot.node_details.meta.gps==null)
        iot.node_details.meta.gps="Unknown"
      else
        iot.node_details.meta.gps=JSON.stringify(iot.node_details.meta.gps)
      if(iot.node_details.meta.type==null)
        iot.node_details.meta.type="Unknown"
      

      var buf = '';

      buf += '<table>';
      buf += '<tr><td class="text-right">';
      buf += 'Global Address';
      buf += '</td><td>';
      buf += iot.node_details.address;
      buf += '</td></tr>';
      buf += '<tr><td class="text-right">';
      buf += 'Short Address';
      buf += '</td><td>';
      buf += iot.node_details._id;
      buf += '</td></tr>';
      buf += '<tr><td class="text-right">';
      buf += 'Parent Address';
      buf += '</td><td>';
      buf += iot.node_details.parent;
      buf += '&nbsp&nbsp<a href="#" onclick="iot.change_parent();">Change</a>'
      buf += '</td></tr>';
      buf += '<tr><td class="text-right">';
      buf += 'EUI-64';
      buf += '</td><td>';
      buf += iot.node_details.eui64;
      buf += '</td></tr>';
      buf += '<tr><td class="text-right">';
      buf += 'Manufacturer';
      buf += '</td><td>';
      buf += 'Texas Instruments'
      buf += '</td></tr>';
      buf += '<tr><td class="text-right">';
      buf += 'GPS Coordination';
      buf += '</td><td>';
      buf += iot.node_details.meta.gps;
      buf += '</td></tr>';
      buf += '<tr><td class="text-right">';
      buf += 'Device Type';
      buf += '</td><td>';
      buf += iot.node_details.meta.type;
      buf += '</td></tr>';
      buf += '<tr><td class="text-right">';
      buf += 'Subscription';
      buf += '</td><td>';
      buf += '<a href="#" id="subscription">Stop</a>'
      buf += '</td></tr>';
      buf += '<tr><td class="text-right">';
      buf += 'Network Performance';
      buf += '</td><td>';
      buf += '<a href="#" onclick="iot.get_diagnosis_info();">Query</a></br><div id="diagnosis_info"></div>'
      buf += '</td></tr>';
      buf += '</table>';

      $("#info_table").html(buf);
      iot.load_tabs();
      
      iot.init_sensor_graph();
      iot.init_power_graph();
      
      iot.load_graphs();
      
      if (!iot.selected_node)
        iot.selected_node = iot.nodes[0]["_id"];
      
      iot.bind_graph_buttons();
      iot.bind_actuator_buttons();
      $("#tab_sensors").click();
      $("#sensors_temperature").click();
      $("#subscription").on("click",function(){
        if($("#subscription").html()=="Stop"){
          if(iot.socket!=null){
            iot.socket.close();
            if(iot.socket.onmessage!=null){
              iot.socket.onmessage({data:"{\"_id\":"+iot.node_details._id+"}"});
            }
            delete iot.socket;
          }
          $("#subscription").html("Start");
        }else{
          iot.load_graphs();
          $("#subscription").html("Stop");
        }
      
      })
      $("#power_details").on("click", function() {
        if(iot.details){
          iot.details=false;
          $(".power_table").hide();
        }else{
          iot.details=true;
          $(".power_table").show();
        }
      });
      $(".power_table").hide();
    }
  });

  $(window).on("resize", function() {
    switch (iot.active.tab) {
      case 'sensors':
        iot.canvas_width = $("#sensor_content").width();
        break;
      case 'power':
        iot.canvas_width = $("#power_content").width();
        break;
      case 'actuator':
        iot.canvas_width = $("#actuator_content").width();
        break;
      default:
        iot.canvas_width = $("#sensor_content").width();
        break;
    }
    
    iot.sensor_canvas["width"] = iot.canvas_width;
    iot.power_canvas["width"] = iot.canvas_width;
  });
  
  $("#tab_sensors").on("click", function(event) {
    $("#tab_power").removeClass("selected");
    $("#tab_actuator").removeClass("selected");
    
    $("#power").hide();
    $("#actuator").hide();
        
    $("#tab_sensors").addClass("selected");
    $("#sensors").show();
    
    iot.canvas_width = $("#sensor_content").width();
    
    if (iot.sensor_canvas)
      iot.sensor_canvas["width"] = iot.canvas_width;
    
    iot.active.tab = 'sensors';
    $("#sensors_temperature").click();
  });
  
  $("#tab_power").on("click", function(event) {
    $("#tab_sensors").removeClass("selected");
    $("#tab_actuator").removeClass("selected");
    
    $("#sensors").hide();
    $("#actuator").hide();
    
    $("#tab_power").addClass("selected");
    $("#power").show();
    
    iot.canvas_width = $("#power_content").width();
    
    if (iot.power_canvas)
      iot.power_canvas["width"] = iot.canvas_width;
    
    iot.active.tab = 'power';
    $("#power_bat").click();
  });
  
  $("#tab_actuator").on("click", function(event) {
    $("#tab_sensors").removeClass("selected");
    $("#tab_power").removeClass("selected");
    
    $("#sensors").hide();
    $("#power").hide();
    
    $("#tab_actuator").addClass("selected");
    $("#actuator").show();
    
    iot.canvas_width = $("#actuator_content").width();
    
    iot.active.tab = 'actuator';
  });


});
