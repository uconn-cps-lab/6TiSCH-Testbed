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

var outer, vis, force;
var node, link, linklabel;
var color = d3.scale.category10();
var overlay = new google.maps.OverlayView();
color.domain(["link","node","gateway","label","stroke","offlinenode"]);

$(document).ready(function(){
  // init svg
  var width=($("#topology").width());
  var height=800;
  outer = d3.select("#topology")
    .append("svg:svg")
    .attr("width", width)
    .attr("height", height)
    .attr("pointer-events", "all");

  vis = outer
    .append('svg:g')
    .append('svg:g');

  vis.append('svg:rect')
    .attr('width', width)
    .attr('height', height)
    .attr('fill', 'white');

  // init force layout
  force = d3.layout.force()
    .size([width, height])
    .linkDistance(100)
    .linkStrength(1)
    .charge(-300)
    .chargeDistance(100)
    .theta(0.1)
    .gravity(0.05)
    .on("tick", tick)


  // get layout properties
  node = vis.selectAll(".node");
  link = vis.selectAll(".link");
  linklabel = vis.selectAll(".linklabel");

  outer.append('svg:defs').append('marker')
    .attr({'id':'arrowhead',
        'viewBox':'-0 -6 12 12',
        'refX':24,
        'refY':0,
        //'markerUnits':'strokeWidth',
        'orient':'auto',
        'markerWidth':12,
        'markerHeight':12,
        'xoverflow':'visible'})
    .append('path')
        .attr('d', 'M 0,-6 L 12 ,0 L 0,6')
        .style("fill", color("label"))

  force.start();
  update();
});

function update() {
  d3.json("nodes", function(error, json) {
    if (error) return console.warn(error);
    var nodes_old = force.nodes();
    var links_old = force.links();
    var nodes_new=[];
    var links_new=[];
    var lifetime_list={};

    for (var idx=0;idx<json.length;++idx) {
      var item=json[idx];
      var node2=+item._id;
      nodes_new.push(node2);
    }

    for (var idx=0;idx<json.length;++idx) {
      var item=json[idx];
      var node2=+item._id;
      var node1=+item.parent;
      lifetime_list[node2]=item.lifetime;
      if(isNaN(node1)||nodes_new.indexOf(node1)==-1){
      }else{
        links_new.push({source: nodes_new.indexOf(node1), target: nodes_new.indexOf(node2), label: item.lifetime, type:"parent"});
      }
      for (var jdx=0;jdx<item.candidate.length;++jdx) {
        var node3=item.candidate[jdx];
        if(nodes_new.indexOf(node3)==-1){
        }else{
          links_new.push({source: nodes_new.indexOf(node3), target: nodes_new.indexOf(node2), label: "", type:"candidate"});
        }
      }
    }
  
    //Find all old attributes
    for(var i=0;i<nodes_new.length;++i){
      var exist=false;
      for(var j=0;j<nodes_old.length;++j){
        if(nodes_new[i]==nodes_old[j].id){
          exist=true;
          break;
        }
      }
      if(exist){
        nodes_new[i]=nodes_old[j];
        nodes_new[i].index=i;
        nodes_new[i].lifetime=lifetime_list[nodes_new[i].id];
      }else{
        nodes_new[i]={id:nodes_new[i],lifetime:lifetime_list[nodes_new[i]]};
      }
    }

    force.nodes(nodes_new);
    force.links(links_new);

    var changed=false;
    for(var idx in nodes_old){
      if(nodes_new.indexOf(nodes_old[idx])==-1)
        changed=true;
    }
    for(var idx in nodes_new){
      if(nodes_old.indexOf(nodes_new[idx])==-1)
        changed=true;
    }
    for(var idx in links_old){
      var found=false;
      for(var jdx in links_new){
        if(links_old[idx].source.id==nodes_new[links_new[jdx].source].id &&
          links_old[idx].target.id==nodes_new[links_new[jdx].target].id &&
          links_old[idx].type==links_new[jdx].type){
          found=true;
          break;
        }
      }
      if(!found)changed=true;
    }
    for(var idx in links_new){
      var found=false;
      for(var jdx in links_old){
        if(links_old[jdx].source.id==nodes_new[links_new[idx].source].id &&
          links_old[jdx].target.id==nodes_new[links_new[idx].target].id &&
          links_old[jdx].type==links_new[idx].type){
          found=true;
          break;
        }
      }
      if(!found)changed=true;
    }
    if(changed){
      force.start();
      redraw();
    }else{
      var cooling=force.alpha();
      force.start();
      force.alpha(cooling);
      redraw();
    }
    setTimeout(update,5000);
  });
}

function tick() {
  link.attr('d', function(d){return 'M '+d.source.x+' '+d.source.y+' L '+ d.target.x +' '+d.target.y})

  node.attr("transform", function(d) { return "translate(" + d.x + "," + d.y + ")"});

  linklabel.attr('transform',function(d,i){
    if (d.target.x<d.source.x){
      bbox = this.getBBox();
      rx = bbox.x+bbox.width/2;
      ry = bbox.y+bbox.height/2;
      return 'rotate(180 '+rx+' '+ry+')';
    } else {
      return 'rotate(0)';
    }
  });
}

// redraw force layout
function redraw() {
  var nodes=force.nodes();
  var links=force.links();


  link = link.data(links);

  link.style("stroke-dasharray", function(d,i){return d.type=="parent"?"1,0":"1,2";})
  link.enter().insert("path",".node")
      .attr("class", "link")
//      .attr("marker-end", "url(#arrowhead)")
      .style("pointer_events", "none")
      .style("stroke", color("link"))
      .style("stroke-dasharray", function(d,i){return d.type=="parent"?"1,0":"1,2";})
      .attr("id", function(d,i){return "path"+i})
  link.exit().remove();

  linklabel = linklabel.data(links);
  linklabel.select("textPath")
      .attr("xlink:href", function(d,i){return "#path"+i})
      .text(function(d){ return d.label });

  linklabel.enter().insert("text")
      .insert("textPath")
      .attr("text-anchor", "middle")
      .attr("alignment-baseline", "central")
      .attr("class", "linklabel")
      .attr("startOffset", "50%")
      .attr("xlink:href", function(d,i){return "#path"+i})
      .text(function(d){ return d.label });
  linklabel.exit().remove();

  node = node.data(nodes);
  node.select("text")
      .text(function(d){return d.id})
  node.select("circle")
      .style("fill", function(d){ return (d.id % 1000)==1? color("gateway"):d.lifetime>0?color("node"):color("offlinenode"); })
      .style("stroke-width", function(d){ if(typeof(iot)!="undefined"&&iot.selected_node!=null)return d.id==iot.selected_node?2:0;else return 0; })

  var node_g = node.enter().insert("g")
      .attr("class", "node")
      .style("cursor","pointer")
      .on("dblclick",function(d){ if(d.id!=1)document.location = 'details.html?id='+d.id })
      .call(force.drag);
  node_g.insert("circle")
      .attr("r", "12")
      .style("fill", function(d){ return (d.id % 1000)==1? color("gateway"):d.lifetime>0?color("node"):color("offlinenode"); })
      .style("stroke",color("stroke"))
      .style("stroke-width", function(d){ 
        if(typeof(iot)!='undefined'&&iot.selected_node!=null)
          return d.id==iot.selected_node?2:0;
        else 
          return 0; })
  node_g.insert("text")
      .attr("text-anchor", "middle")
      .attr("alignment-baseline", "central")
//      .attr("dy", -11)
      .text(function(d){return d.id})
  node_g.transition()
      .duration(250)
      .ease("elastic")
      .attr("r", 6.5);

  node.exit().transition()
      .attr("r", 0)
      .remove();


  if (d3.event) {
    // prevent browser's default behavior
    //d3.event.preventDefault();
  }


}
