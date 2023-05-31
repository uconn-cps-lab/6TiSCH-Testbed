// hello.js
const addon                   = require('./build/Release/addon');
const ecc = require('./ecc');
const crypto = require('./crypto');

var rk=[];
var Nr = 7;
var src=[];
var dst=[];

for (var i = 0; i < 4*(Nr + 1); i++)
{
   rk[i] = i*10000;
}

for (var i = 0; i < 16; i++)
{
   src[i] = 100*i;
}

var ctx ={"ek":rk,"Nr":Nr};

crypto.rijndael_encrypt(ctx, src, dst);

console.log(dst);
