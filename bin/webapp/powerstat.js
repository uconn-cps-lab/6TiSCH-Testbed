var power_dict={};
const T = 1 * 60 * 1000 // RC (time constant) in millisecond

// id, power
function add_point(id, power){
  var ts = Date.now();
  if(power_dict[id]==null){
    power_dict[id] = power;
    power_dict[id].ts = ts;
  }else{
    var dt = ts - power_dict[id].ts;
    var alpha = dt/(T+dt);
    for(var item in power){
      power_dict[id][item]=(1-alpha)*power_dict[id][item]+alpha*power[item]
    }
    power_dict[id].ts = ts;
  }
}
function get_dict(){
  return power_dict;
}

module.exports={
  add_point:add_point,
  get_dict:get_dict
}
