// callcecc.cc
#include <node.h>

namespace demo {

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;
using v8::Array;
using v8::Number;

extern "C" void ecc_ec_double(const uint32_t *px, const uint32_t *py, uint32_t *Dx, uint32_t *Dy);
extern "C" int ecc_is_valid_key(const uint32_t * priv_key);
extern "C" int ecc_ecdsa_validate(const uint32_t *x, const uint32_t *y, const uint32_t *e, const uint32_t *r, const uint32_t *s);
extern "C" int ecc_ecdsa_sign(const uint32_t *d, const uint32_t *e, const uint32_t *k, uint32_t *r, uint32_t *s);
extern "C" void ecc_ec_mult(const uint32_t *px, const uint32_t *py, const uint32_t *secret, uint32_t *resultx, uint32_t *resulty);

void callc_ecc_is_valid_key(const FunctionCallbackInfo<Value>& args) 
{
  Isolate* isolate = args.GetIsolate();
  Local<Array> priv_key_in = Local<Array>::Cast(args[0]);
  uint32_t priv_key[8];
  
  for (int i = 0; i < 8; i++)
  {
     priv_key[i] = priv_key_in->Get(i)->NumberValue();
  }
  
  int retv = ecc_is_valid_key(priv_key);
  
  args.GetReturnValue().Set(Number::New(isolate, retv));
}

void callc_ecc_ecdsa_validate(const FunctionCallbackInfo<Value>& args) 
{
  Isolate* isolate = args.GetIsolate();
  Local<Array> x_in = Local<Array>::Cast(args[0]);
  Local<Array> y_in = Local<Array>::Cast(args[1]);
  Local<Array> e_in = Local<Array>::Cast(args[2]);
  Local<Array> r_in = Local<Array>::Cast(args[3]);
  Local<Array> s_in = Local<Array>::Cast(args[4]);
  uint32_t x[8], y[8], e[8], r[8], s[8];
  
  for (int i = 0; i < 8; i++)
  {
     x[i] = x_in->Get(i)->NumberValue();
     y[i] = y_in->Get(i)->NumberValue();
     e[i] = e_in->Get(i)->NumberValue();
     r[i] = r_in->Get(i)->NumberValue();
     s[i] = s_in->Get(i)->NumberValue();
  }
  
  int retv = ecc_ecdsa_validate(x, y, e, r, s);
  
  args.GetReturnValue().Set(Number::New(isolate, retv));
}

void callc_ecc_ecdsa_sign(const FunctionCallbackInfo<Value>& args) 
{
  Isolate* isolate = args.GetIsolate();
  Local<Array> d_in = Local<Array>::Cast(args[0]);
  Local<Array> e_in = Local<Array>::Cast(args[1]);
  Local<Array> k_in = Local<Array>::Cast(args[2]);
  Local<Array> r_out = Array::New(isolate);
  Local<Array> s_out = Array::New(isolate);
  Local<Object> ret_obj = Object::New(isolate);

  
  uint32_t d[8], e[8], k[8];
  uint32_t r[9], s[9];
  
  for (int i = 0; i < 8; i++)
  {
     d[i] = d_in->Get(i)->NumberValue();
     e[i] = e_in->Get(i)->NumberValue();
     k[i] = k_in->Get(i)->NumberValue();
  }
  
  int retv = ecc_ecdsa_sign(d, e, k, r, s);
  
  for (int i = 0; i < 9; i++)
  {
     r_out->Set(i, Number::New(isolate, r[i]));
     s_out->Set(i, Number::New(isolate, s[i]));
  }
  
  
  ret_obj->Set(String::NewFromUtf8(isolate, "ret"), Number::New(isolate, retv));
  ret_obj->Set(String::NewFromUtf8(isolate, "r"), r_out);
  ret_obj->Set(String::NewFromUtf8(isolate, "s"), s_out);
  
  args.GetReturnValue().Set(ret_obj);
}

void callc_ecc_ec_mult(const FunctionCallbackInfo<Value>& args) 
{
  Isolate* isolate = args.GetIsolate();
  Local<Array> px_in = Local<Array>::Cast(args[0]);
  Local<Array> py_in = Local<Array>::Cast(args[1]);
  Local<Array> secret_in = Local<Array>::Cast(args[2]);
  Local<Array> resultx_out = Array::New(isolate);
  Local<Array> resulty_out = Array::New(isolate);
  Local<Object> ret_obj = Object::New(isolate);
  
  uint32_t px[8], py[8], secret[8];
  uint32_t resultx[8], resulty[8];
  
  for (int i = 0; i < 8; i++)
  {
     px[i] = px_in->Get(i)->NumberValue();
     py[i] = py_in->Get(i)->NumberValue();
     secret[i] = secret_in->Get(i)->NumberValue();
  }
  
  ecc_ec_mult(px, py, secret, resultx, resulty);
  
  for (int i = 0; i < 8; i++)
  {
     resultx_out->Set(i, Number::New(isolate, resultx[i]));
     resulty_out->Set(i, Number::New(isolate, resulty[i]));
  }
  
  ret_obj->Set(String::NewFromUtf8(isolate, "resultx"), resultx_out);
  ret_obj->Set(String::NewFromUtf8(isolate, "resulty"), resulty_out);
  
  args.GetReturnValue().Set(ret_obj);
}

void init(Local<Object> exports) { 
  NODE_SET_METHOD(exports, "ecc_is_valid_key", callc_ecc_is_valid_key);
  NODE_SET_METHOD(exports, "ecc_ecdsa_validate", callc_ecc_ecdsa_validate);
  NODE_SET_METHOD(exports, "ecc_ecdsa_sign", callc_ecc_ecdsa_sign);
  NODE_SET_METHOD(exports, "ecc_ec_mult", callc_ecc_ec_mult);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, init)

}  // namespace demo