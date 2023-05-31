
const addon                   = require('./build/Release/addon');
var memcpy                    = require('dtls/dtls_config').memcpy;

// copy one array to another
function copy(from, to, length)
{
   memcpy(to, from, length);
}

function ecc_ec_mult(px, py, secret, resultx, resulty)
{
   var result = addon.ecc_ec_mult(px, py, secret);
   copy(result.resultx, resultx, result.resultx.length);
   copy(result.resulty, resulty, result.resulty.length);
}

function ecc_ecdsa_sign(d, e, k, r, s)
{
   var result = addon.ecc_ecdsa_sign(d, e, k);
   copy(result.r, r, result.r.length);
   copy(result.s, s, result.s.length);
   return result.ret;
}

function ecc_ecdsa_validate(x, y, e, r, s)
{
   return addon.ecc_ecdsa_validate(x, y, e, r, s);
}

function ecc_is_valid_key(priv_key)
{
   return addon.ecc_is_valid_key(priv_key);
}

function ecc_ecdh(px, py, secret, resultx, resulty) 
{
	ecc_ec_mult(px, py, secret, resultx, resulty);
}

function ecc_gen_pub_key(priv_key, pub_x, pub_y)
{
	ecc_ec_mult(ecc_g_point_x, ecc_g_point_y, priv_key, pub_x, pub_y);
}

module.exports.ecc_ec_mult                   = ecc_ec_mult;
module.exports.ecc_ecdsa_sign                = ecc_ecdsa_sign;
module.exports.ecc_ecdsa_validate            = ecc_ecdsa_validate;
module.exports.ecc_is_valid_key              = ecc_is_valid_key;
module.exports.ecc_ecdh                      = ecc_ecdh;
module.exports.ecc_gen_pub_key               = ecc_gen_pub_key;
