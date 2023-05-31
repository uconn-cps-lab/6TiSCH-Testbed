var topology_interface = require('./topology_interface');
var http = require("http");
var topology = {
  "16":10,
  "15":12,
  "14":11,
  "13":4,
  "12":16,
  "11":9,
  "10":1,
  "9":16,
  "8":1,
  "7":10,
  "6":15,
  "5":9,
  "4":1,
  "3":12
}
function id2address(id){
  return "2001:db8:1234:ffff::ff:fe00:"+(+id).toString(16);
}

var seq = [];
if(process.argv.length>=3){
  for(var i=2;i<process.argv.length;++i){
    var id = process.argv[2];
    if(topology[id]){
      topology_interface.change(topology[id], id2address(id));
    }else{
      console.log("Parent of "+id+" isn't specified!");
    }
  }
}else{
  var dfs = function(id){
    if(topology[id]){
        //topology_interface.change(topology[id], id2address(id));
        seq.push(id);
    }
    for(var i in topology){
      if(topology[i]==id){
        dfs(i);
      }
    }
  }
  dfs(1);
  console.log(seq);

  var timer;
  var check_topology = function(){
    http.get("http://localhost/nodes", res => {
      res.setEncoding("utf8");
      var body = "";
      res.on("data", data => {
        body += data;
      });
      res.on("end", () => {
        var json = JSON.parse(body);
        for(var i=0;i<seq.length;++i){//check from sequence list
          for(var j=0;j<json.length;++j){
            if(seq[i]==json[j]._id){
              if(topology[seq[i]]!=json[j].parent){
                topology_interface.change(topology[seq[i]], id2address(seq[i]));
                return;
              }
            }
          }
        }
        clearInterval(timer);
        process.exit();
        });
    });
  }
  var timer = setInterval(function(){ check_topology();
  }, 60 * 1000);
  check_topology();
}

/*
topology_interface.change(3, "2001:db8:1234:ffff::ff:fe00:10");
topology_interface.change(3, "2001:db8:1234:ffff::ff:fe00:f");
topology_interface.change(3, "2001:db8:1234:ffff::ff:fe00:e");
topology_interface.change(3, "2001:db8:1234:ffff::ff:fe00:d");
topology_interface.change(4, "2001:db8:1234:ffff::ff:fe00:c");
topology_interface.change(4, "2001:db8:1234:ffff::ff:fe00:b");
topology_interface.change(4, "2001:db8:1234:ffff::ff:fe00:a");
topology_interface.change(5, "2001:db8:1234:ffff::ff:fe00:9");
topology_interface.change(5, "2001:db8:1234:ffff::ff:fe00:8");
topology_interface.change(6, "2001:db8:1234:ffff::ff:fe00:7");
*/
