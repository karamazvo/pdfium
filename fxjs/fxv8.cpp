// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/fxv8.h"

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "v8/include/v8-container.h"
#include "v8/include/v8-date.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-primitive.h"
#include "v8/include/v8-value.h"

namespace fxv8 {

bool IsUndefined(v8::Local<v8::Value> value) {
  return !value.IsEmpty() && value->IsUndefined();
}

bool IsNull(v8::Local<v8::Value> value) {
  return !value.IsEmpty() && value->IsNull();
}

bool IsBoolean(v8::Local<v8::Value> value) {
  return !value.IsEmpty() && value->IsBoolean();
}

bool IsString(v8::Local<v8::Value> value) {
  return !value.IsEmpty() && value->IsString();
}

bool IsNumber(v8::Local<v8::Value> value) {
  return !value.IsEmpty() && value->IsNumber();
}

bool IsInteger(v8::Local<v8::Value> value) {
  return !value.IsEmpty() && value->IsInt32();
}

bool IsObject(v8::Local<v8::Value> value) {
  return !value.IsEmpty() && value->IsObject();
}

bool IsArray(v8::Local<v8::Value> value) {
  return !value.IsEmpty() && value->IsArray();
}

bool IsDate(v8::Local<v8::Value> value) {
  return !value.IsEmpty() && value->IsDate();
}

bool IsFunction(v8::Local<v8::Value> value) {
  return !value.IsEmpty() && value->IsFunction();
}

v8::Local<v8::Value> NewNullHelper(v8::Isolate* isolate) {
  return v8::Null(isolate);
}

v8::Local<v8::Value> NewUndefinedHelper(v8::Isolate* isolate) {
  return v8::Undefined(isolate);
}

v8::Local<v8::Number> NewNumberHelper(v8::Isolate* isolate, int number) {
  return v8::Int32::New(isolate, number);
}

v8::Local<v8::Number> NewNumberHelper(v8::Isolate* isolate, double number) {
  return v8::Number::New(isolate, number);
}

v8::Local<v8::Number> NewNumberHelper(v8::Isolate* isolate, float number) {
  return v8::Number::New(isolate, number);
}

v8::Local<v8::Boolean> NewBooleanHelper(v8::Isolate* isolate, bool b) {
  return v8::Boolean::New(isolate, b);
}

v8::Local<v8::String> NewStringHelper(v8::Isolate* isolate,
                                      ByteStringView str) {
  return v8::String::NewFromUtf8(isolate, str.unterminated_c_str(),
                                 v8::NewStringType::kNormal,
                                 pdfium::checked_cast<int>(str.GetLength()))
      .ToLocalChecked();
}

v8::Local<v8::String> NewStringHelper(v8::Isolate* isolate,
                                      WideStringView str) {
  return NewStringHelper(isolate, FX_UTF8Encode(str).AsStringView());
}

v8::Local<v8::Array> NewArrayHelper(v8::Isolate* isolate) {
  return v8::Array::New(isolate);
}

v8::Local<v8::Array> NewArrayHelper(v8::Isolate* isolate,
                                    pdfium::span<v8::Local<v8::Value>> values) {
  v8::Local<v8::Array> result = NewArrayHelper(isolate);
  for (size_t i = 0; i < values.size(); ++i) {
    fxv8::ReentrantPutArrayElementHelper(
        isolate, result, i,
        values[i].IsEmpty() ? fxv8::NewUndefinedHelper(isolate) : values[i]);
  }
  return result;
}

v8::Local<v8::Object> NewObjectHelper(v8::Isolate* isolate) {
  return v8::Object::New(isolate);
}

v8::Local<v8::Date> NewDateHelper(v8::Isolate* isolate, double d) {
  return v8::Date::New(isolate->GetCurrentContext(), d)
      .ToLocalChecked()
      .As<v8::Date>();
}

WideString ToWideString(v8::Isolate* isolate, v8::Local<v8::String> pValue) {
  v8::String::Utf8Value s(isolate, pValue);
  // SAFETY: required from V8.
  return WideString::FromUTF8(UNSAFE_BUFFERS(ByteStringView(*s, s.length())));
}

ByteString ToByteString(v8::Isolate* isolate, v8::Local<v8::String> pValue) {
  v8::String::Utf8Value s(isolate, pValue);
  // SAFETY: required from V8.
  return UNSAFE_BUFFERS(ByteString(*s, s.length()));
}

int ReentrantToInt32Helper(v8::Isolate* isolate, v8::Local<v8::Value> pValue) {
  if (pValue.IsEmpty()) {
    return 0;
  }
  v8::TryCatch squash_exceptions(isolate);
  return pValue->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
}

bool ReentrantToBooleanHelper(v8::Isolate* isolate,
                              v8::Local<v8::Value> pValue) {
  if (pValue.IsEmpty()) {
    return false;
  }
  v8::TryCatch squash_exceptions(isolate);
  return pValue->BooleanValue(isolate);
}

float ReentrantToFloatHelper(v8::Isolate* isolate,
                             v8::Local<v8::Value> pValue) {
  return static_cast<float>(ReentrantToDoubleHelper(isolate, pValue));
}

double ReentrantToDoubleHelper(v8::Isolate* isolate,
                               v8::Local<v8::Value> pValue) {
  if (pValue.IsEmpty()) {
    return 0.0;
  }
  v8::TryCatch squash_exceptions(isolate);
  return pValue->NumberValue(isolate->GetCurrentContext()).FromMaybe(0.0);
}

WideString ReentrantToWideStringHelper(v8::Isolate* isolate,
                                       v8::Local<v8::Value> pValue) {
  if (pValue.IsEmpty()) {
    return WideString();
  }

  v8::TryCatch squash_exceptions(isolate);
  v8::MaybeLocal<v8::String> maybe_string =
      pValue->ToString(isolate->GetCurrentContext());
  if (maybe_string.IsEmpty()) {
    return WideString();
  }

  return ToWideString(isolate, maybe_string.ToLocalChecked());
}

ByteString ReentrantToByteStringHelper(v8::Isolate* isolate,
                                       v8::Local<v8::Value> pValue) {
  if (pValue.IsEmpty()) {
    return ByteString();
  }

  v8::TryCatch squash_exceptions(isolate);
  v8::MaybeLocal<v8::String> maybe_string =
      pValue->ToString(isolate->GetCurrentContext());
  if (maybe_string.IsEmpty()) {
    return ByteString();
  }

  return ToByteString(isolate, maybe_string.ToLocalChecked());
}

v8::Local<v8::Object> ReentrantToObjectHelper(v8::Isolate* isolate,
                                              v8::Local<v8::Value> pValue) {
  if (!fxv8::IsObject(pValue)) {
    return v8::Local<v8::Object>();
  }

  v8::TryCatch squash_exceptions(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  return pValue->ToObject(context).ToLocalChecked();
}

v8::Local<v8::Array> ReentrantToArrayHelper(v8::Isolate* isolate,
                                            v8::Local<v8::Value> pValue) {
  if (!fxv8::IsArray(pValue)) {
    return v8::Local<v8::Array>();
  }

  v8::TryCatch squash_exceptions(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  return v8::Local<v8::Array>::Cast(pValue->ToObject(context).ToLocalChecked());
}

v8::Local<v8::Value> ReentrantGetObjectPropertyHelper(
    v8::Isolate* isolate,
    v8::Local<v8::Object> pObj,
    ByteStringView bsUTF8PropertyName) {
  if (pObj.IsEmpty()) {
    return v8::Local<v8::Value>();
  }

  v8::TryCatch squash_exceptions(isolate);
  v8::Local<v8::Value> val;
  if (!pObj->Get(isolate->GetCurrentContext(),
                 NewStringHelper(isolate, bsUTF8PropertyName))
           .ToLocal(&val)) {
    return v8::Local<v8::Value>();
  }
  return val;
}

std::vector<WideString> ReentrantGetObjectPropertyNamesHelper(
    v8::Isolate* isolate,
    v8::Local<v8::Object> pObj) {
  if (pObj.IsEmpty()) {
    return std::vector<WideString>();
  }

  v8::TryCatch squash_exceptions(isolate);
  v8::Local<v8::Array> val;
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  if (!pObj->GetPropertyNames(context).ToLocal(&val)) {
    return std::vector<WideString>();
  }

  std::vector<WideString> result;
  for (uint32_t i = 0; i < val->Length(); ++i) {
    result.push_back(ReentrantToWideStringHelper(
        isolate, val->Get(context, i).ToLocalChecked()));
  }
  return result;
}

bool ReentrantHasObjectOwnPropertyHelper(v8::Isolate* isolate,
                                         v8::Local<v8::Object> pObj,
                                         ByteStringView bsUTF8PropertyName) {
  if (pObj.IsEmpty()) {
    return false;
  }

  v8::TryCatch squash_exceptions(isolate);
  v8::Local<v8::Context> pContext = isolate->GetCurrentContext();
  v8::Local<v8::String> hKey =
      fxv8::NewStringHelper(isolate, bsUTF8PropertyName);
  return pObj->HasRealNamedProperty(pContext, hKey).FromJust();
}

bool ReentrantSetObjectOwnPropertyHelper(v8::Isolate* isolate,
                                         v8::Local<v8::Object> pObj,
                                         ByteStringView bsUTF8PropertyName,
                                         v8::Local<v8::Value> pValue) {
  if (pObj.IsEmpty() || pValue.IsEmpty()) {
    return false;
  }

  v8::TryCatch squash_exceptions(isolate);
  v8::Local<v8::String> name = NewStringHelper(isolate, bsUTF8PropertyName);
  return pObj->DefineOwnProperty(isolate->GetCurrentContext(), name, pValue)
      .FromMaybe(false);
}

bool ReentrantPutObjectPropertyHelper(v8::Isolate* isolate,
                                      v8::Local<v8::Object> pObj,
                                      ByteStringView bsUTF8PropertyName,
                                      v8::Local<v8::Value> pPut) {
  if (pObj.IsEmpty() || pPut.IsEmpty()) {
    return false;
  }

  v8::TryCatch squash_exceptions(isolate);
  v8::Local<v8::String> name = NewStringHelper(isolate, bsUTF8PropertyName);
  v8::Maybe<bool> result = pObj->Set(isolate->GetCurrentContext(), name, pPut);
  return result.IsJust() && result.FromJust();
}

void ReentrantDeleteObjectPropertyHelper(v8::Isolate* isolate,
                                         v8::Local<v8::Object> pObj,
                                         ByteStringView bsUTF8PropertyName) {
  v8::TryCatch squash_exceptions(isolate);
  pObj->Delete(isolate->GetCurrentContext(),
               fxv8::NewStringHelper(isolate, bsUTF8PropertyName))
      .FromJust();
}

bool ReentrantPutArrayElementHelper(v8::Isolate* isolate,
                                    v8::Local<v8::Array> pArray,
                                    size_t index,
                                    v8::Local<v8::Value> pValue) {
  if (pArray.IsEmpty()) {
    return false;
  }

  v8::TryCatch squash_exceptions(isolate);
  v8::Maybe<bool> result =
      pArray->Set(isolate->GetCurrentContext(),
                  pdfium::checked_cast<uint32_t>(index), pValue);
  return result.IsJust() && result.FromJust();
}

v8::Local<v8::Value> ReentrantGetArrayElementHelper(v8::Isolate* isolate,
                                                    v8::Local<v8::Array> pArray,
                                                    size_t index) {
  if (pArray.IsEmpty()) {
    return v8::Local<v8::Value>();
  }

  v8::TryCatch squash_exceptions(isolate);
  v8::Local<v8::Value> val;
  if (!pArray
           ->Get(isolate->GetCurrentContext(),
                 pdfium::checked_cast<uint32_t>(index))
           .ToLocal(&val)) {
    return v8::Local<v8::Value>();
  }
  return val;
}

size_t GetArrayLengthHelper(v8::Local<v8::Array> pArray) {
  if (pArray.IsEmpty()) {
    return 0;
  }
  return pArray->Length();
}

void ThrowExceptionHelper(v8::Isolate* isolate, ByteStringView str) {
  isolate->ThrowException(NewStringHelper(isolate, str));
}

void ThrowExceptionHelper(v8::Isolate* isolate, WideStringView str) {
  isolate->ThrowException(NewStringHelper(isolate, str));
}

}  // namespace fxv8
