var memwatch = require('node-memwatch');
memwatch.on('leak', (info)=>{
  console.log(info)
})
