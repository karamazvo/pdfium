// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CFXJSE_VALUE_H_
#define FXJS_XFA_CFXJSE_VALUE_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "core/fxcrt/check.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/unowned_ptr.h"
#include "v8/include/v8-forward.h"
#include "v8/include/v8-persistent-handle.h"

class CFXJSE_Class;
class CFXJSE_HostObject;

class CFXJSE_Value {
 public:
  CFXJSE_Value();
  CFXJSE_Value(v8::Isolate* isolate, v8::Local<v8::Value> value);
  ~CFXJSE_Value();

  bool IsEmpty() const;
  bool IsUndefined(v8::Isolate* isolate) const;
  bool IsNull(v8::Isolate* isolate) const;
  bool IsBoolean(v8::Isolate* isolate) const;
  bool IsString(v8::Isolate* isolate) const;
  bool IsNumber(v8::Isolate* isolate) const;
  bool IsInteger(v8::Isolate* isolate) const;
  bool IsObject(v8::Isolate* isolate) const;
  bool IsArray(v8::Isolate* isolate) const;
  bool IsFunction(v8::Isolate* isolate) const;
  bool ToBoolean(v8::Isolate* isolate) const;
  float ToFloat(v8::Isolate* isolate) const;
  double ToDouble(v8::Isolate* isolate) const;
  int32_t ToInteger(v8::Isolate* isolate) const;
  ByteString ToString(v8::Isolate* isolate) const;
  WideString ToWideString(v8::Isolate* isolate) const {
    return WideString::FromUTF8(ToString(isolate).AsStringView());
  }
  CFXJSE_HostObject* ToHostObject(v8::Isolate* isolate) const;

  void SetUndefined(v8::Isolate* isolate);
  void SetNull(v8::Isolate* isolate);
  void SetBoolean(v8::Isolate* isolate, bool bBoolean);
  void SetInteger(v8::Isolate* isolate, int32_t nInteger);
  void SetDouble(v8::Isolate* isolate, double dDouble);
  void SetString(v8::Isolate* isolate, ByteStringView szString);
  void SetFloat(v8::Isolate* isolate, float fFloat);

  void SetHostObject(v8::Isolate* isolate,
                     CFXJSE_HostObject* pObject,
                     CFXJSE_Class* pClass);

  void SetArray(v8::Isolate* isolate,
                const std::vector<std::unique_ptr<CFXJSE_Value>>& values);

  bool GetObjectProperty(v8::Isolate* isolate,
                         ByteStringView szPropName,
                         CFXJSE_Value* pPropValue);
  bool SetObjectProperty(v8::Isolate* isolate,
                         ByteStringView szPropName,
                         CFXJSE_Value* pPropValue);
  bool GetObjectPropertyByIdx(v8::Isolate* isolate,
                              uint32_t uPropIdx,
                              CFXJSE_Value* pPropValue);
  void DeleteObjectProperty(v8::Isolate* isolate, ByteStringView szPropName);
  bool SetObjectOwnProperty(v8::Isolate* isolate,
                            ByteStringView szPropName,
                            CFXJSE_Value* pPropValue);

  // Return empty local on error.
  static v8::Local<v8::Function> NewBoundFunction(
      v8::Isolate* isolate,
      v8::Local<v8::Function> hOldFunction,
      v8::Local<v8::Object> lpNewThis);

  v8::Local<v8::Value> GetValue(v8::Isolate* isolate) const;
  const v8::Global<v8::Value>& DirectGetValue() const { return value_; }
  void ForceSetValue(v8::Isolate* isolate, v8::Local<v8::Value> hValue) {
    value_.Reset(isolate, hValue);
  }

 private:
  CFXJSE_Value(const CFXJSE_Value&) = delete;
  CFXJSE_Value& operator=(const CFXJSE_Value&) = delete;

  v8::Global<v8::Value> value_;
};

#endif  // FXJS_XFA_CFXJSE_VALUE_H_
