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

var nodes=[];
var links=[];
var svg;
var overlay=null;
var color = d3.scale.category10();
var viewbox={minx:0,miny:0,maxx:0,maxy:0}
var margin = 20;
var map=null;
var set_center=true;
color.domain(["link","node","gateway","label","stroke","offlinenode"]);
$(document).ready(function(){
  var width=($("#topology").width());
  var height=900;
  $("#topology").height(height);

  map = new google.maps.Map(d3.select("#topology").node(), {
    center: {lat: -34.397, lng: 150.644},
    zoom: 20//random center and zoom to display, we will reset later
  });
  overlay = new google.maps.OverlayView();

  overlay.onAdd = function() {
    svg = d3.select(overlay.getPanes().overlayMouseTarget).append("svg")
    overlay.draw = draw;
    update();
  };

  overlay.onRemove = function() {
    d3.selectAll("#svgoverlay").remove();
  }
  overlay.setMap(map);

  d3.json("indoor/list.json", function(error,json){
    if(error)throw error;
    for(var name in json){
      var item=json[name];
      var indoor = new google.maps.GroundOverlay(
          item.img,
          {
            north:item.latlng[0],
            south:item.latlng[2],
            west:item.latlng[1],
            east:item.latlng[3]
          }
      );
      indoor.setOpacity(0.3);
      indoor.setMap(map);
    }
  })
});
function update(){
  d3.json("nodes", function(error,json){
    if(error)throw error;
    nodes=json;
    links=[];
    for (var idx=0;idx<json.length;++idx) {
      var item=json[idx];
      var target;
      if(item.lifetime<=0)continue;
      for(target=0;target<nodes.length;++target){
        if(nodes[target]._id==item.parent){
          break;
        }
      }
      if(target>=nodes.length)continue;
      links.push({source: nodes[idx], target: nodes[target]});
    }
    if(set_center){
      var bound = new google.maps.LatLngBounds();
      for (var idx=0;idx<nodes.length;++idx) {
        if(nodes[idx].meta.gps!=null){
          bound.extend(new google.maps.LatLng(nodes[idx].meta.gps[0],nodes[idx].meta.gps[1]));
        }
      }
      map.fitBounds(bound);

      set_center=false;
    }
    draw();
    setTimeout(update,5000);
  })
}
function draw(){
  var projection = overlay.getProjection();

  //calculate projected coordination and viewbox
  for(var i=0;i<nodes.length;++i){
    d=nodes[i];
    if(d.meta.gps==null)continue;
    var c = new google.maps.LatLng(d.meta.gps[0], d.meta.gps[1]);
    var p = projection.fromLatLngToDivPixel(c);
    d.x=p.x;
    d.y=p.y;
    if(i==0){
      viewbox.minx=viewbox.maxx=d.x;
      viewbox.miny=viewbox.maxy=d.y;
    }else{
      if(d.x>viewbox.maxx)viewbox.maxx=d.x;
      if(d.y>viewbox.maxy)viewbox.maxy=d.y;
      if(d.x<viewbox.minx)viewbox.minx=d.x;
      if(d.y<viewbox.miny)viewbox.miny=d.y;
    }
  }
  viewbox.minx-=margin;
  viewbox.miny-=margin;
  viewbox.maxx+=margin;
  viewbox.maxy+=margin;

  svg.attr("id", "svgoverlay")
    .style("position", "absolute")
    .style("left",viewbox.minx+"px")
    .style("top",viewbox.miny+"px")
    .attr("width",viewbox.maxx-viewbox.minx+"px")
    .attr("height",viewbox.maxy-viewbox.miny+"px")
/*    svg.append("rect")
      .attr("width","100%")
      .attr("height","100%")
      .attr("fill","blue")
      .attr("fill-opacity","0.4")*/

  var nodes_data = svg.selectAll("g#node").data(nodes);
  nodes_data.exit().remove();
  nodes_data.each(draw_node);
  nodes_data.enter().append("g").attr("id","node").each(draw_node);

  var links_data = svg.selectAll("g#link").data(links);
  links_data.exit().remove();
  links_data.each(draw_link);
  links_data.enter().insert("g","#node").attr("id","link").each(draw_link);

  function draw_node(d){
    d3.select(this).selectAll("*").remove();
    if(d.meta.gps==null)return;

    d3.select(this)
      .style("cursor","pointer")
      .attr("transform", function(d) {
        return "translate(" + (d.x-viewbox.minx) + "," + (d.y-viewbox.miny) + ")"
      })
     .on("click",function(d){ document.location = 'details.html?id='+d._id })

    d3.select(this).append("circle")
      .attr("r", "12")
      .style("fill", function(d){ return (d._id % 1000)==1? color("gateway"):d.lifetime>-300?color("node"):color("offlinenode"); })
      .style("stroke",color("stroke"))
      .style("stroke-width", function(d){ if(typeof(iot)!="undefined"&&iot.selected_node!=null)return d._id==iot.selected_node?2:0;else return 0; })

    d3.select(this).append("text")
      .attr("text-anchor", "middle")
      .attr("alignment-baseline", "central")
      .text(function(d){return d._id});
  }

  function draw_link(d,i){
    d3.select(this).selectAll("*").remove();
    if(d.source.meta.gps==null||d.target.meta.gps==null)return;

    d3.select(this).append("path")
      .attr("stroke", color("link"))
      .attr("id", function(d){return "path"+i})
      .attr('d', function(d){
        return 'M '+(d.source.x-viewbox.minx)+' '+(d.source.y-viewbox.miny)+' L '+ (d.target.x-viewbox.minx) +' '+(d.target.y-viewbox.miny)
      })

    d3.select(this).append("text")
      .attr('transform',function(d){
        if (d.target.x<d.source.x){
          rx = (d.target.x+d.source.x)/2-viewbox.minx;
          ry = (d.target.y+d.source.y)/2-viewbox.miny;
          return 'rotate(180 '+rx+' '+ry+')';
        } else {
          return 'rotate(0)';
        }
      })
      .append("textPath")
      .attr("text-anchor", "middle")
      .attr("alignment-baseline", "central")
      .attr("startOffset", "50%")
      .attr("xlink:href", function(d){return "#path"+i})
      .text(function(d){ return d.label })
  }

}
