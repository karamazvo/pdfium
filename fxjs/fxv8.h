// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_FXV8_H_
#define FXJS_FXV8_H_

#include <stddef.h>

#include <vector>

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/span.h"
#include "v8/include/v8-forward.h"

// The fxv8 functions soften up the interface to the V8 API. In particular,
// PDFium uses size_t for sizes and indices, but V8 mostly uses ints, so
// these routines perform checked conversions.

namespace fxv8 {

// These first check for empty locals.
bool IsUndefined(v8::Local<v8::Value> value);
bool IsNull(v8::Local<v8::Value> value);
bool IsBoolean(v8::Local<v8::Value> value);
bool IsString(v8::Local<v8::Value> value);
bool IsNumber(v8::Local<v8::Value> value);
bool IsInteger(v8::Local<v8::Value> value);
bool IsObject(v8::Local<v8::Value> value);
bool IsArray(v8::Local<v8::Value> value);
bool IsDate(v8::Local<v8::Value> value);
bool IsFunction(v8::Local<v8::Value> value);

v8::Local<v8::Value> NewNullHelper(v8::Isolate* isolate);
v8::Local<v8::Value> NewUndefinedHelper(v8::Isolate* isolate);
v8::Local<v8::Number> NewNumberHelper(v8::Isolate* isolate, int number);
v8::Local<v8::Number> NewNumberHelper(v8::Isolate* isolate, double number);
v8::Local<v8::Number> NewNumberHelper(v8::Isolate* isolate, float number);
v8::Local<v8::Boolean> NewBooleanHelper(v8::Isolate* isolate, bool b);
v8::Local<v8::String> NewStringHelper(v8::Isolate* isolate, ByteStringView str);
v8::Local<v8::String> NewStringHelper(v8::Isolate* isolate, WideStringView str);
v8::Local<v8::Array> NewArrayHelper(v8::Isolate* isolate);
v8::Local<v8::Array> NewArrayHelper(v8::Isolate* isolate,
                                    pdfium::span<v8::Local<v8::Value>> values);
v8::Local<v8::Object> NewObjectHelper(v8::Isolate* isolate);
v8::Local<v8::Date> NewDateHelper(v8::Isolate* isolate, double d);

// Conversion to PDFium type without re-entry from known v8 type.
WideString ToWideString(v8::Isolate* isolate, v8::Local<v8::String> value);
ByteString ToByteString(v8::Isolate* isolate, v8::Local<v8::String> value);

// Conversion to PDFium type with possible re-entry for coercion.
int32_t ReentrantToInt32Helper(v8::Isolate* isolate,
                               v8::Local<v8::Value> value);
bool ReentrantToBooleanHelper(v8::Isolate* isolate, v8::Local<v8::Value> value);
float ReentrantToFloatHelper(v8::Isolate* isolate, v8::Local<v8::Value> value);
double ReentrantToDoubleHelper(v8::Isolate* isolate,
                               v8::Local<v8::Value> value);
WideString ReentrantToWideStringHelper(v8::Isolate* isolate,
                                       v8::Local<v8::Value> value);
ByteString ReentrantToByteStringHelper(v8::Isolate* isolate,
                                       v8::Local<v8::Value> value);
v8::Local<v8::Object> ReentrantToObjectHelper(v8::Isolate* isolate,
                                              v8::Local<v8::Value> value);
v8::Local<v8::Array> ReentrantToArrayHelper(v8::Isolate* isolate,
                                            v8::Local<v8::Value> value);

v8::Local<v8::Value> ReentrantGetObjectPropertyHelper(
    v8::Isolate* isolate,
    v8::Local<v8::Object> obj,
    ByteStringView bsUTF8PropertyName);
std::vector<WideString> ReentrantGetObjectPropertyNamesHelper(
    v8::Isolate* isolate,
    v8::Local<v8::Object> obj);
bool ReentrantHasObjectOwnPropertyHelper(v8::Isolate* isolate,
                                         v8::Local<v8::Object> obj,
                                         ByteStringView bsUTF8PropertyName);
bool ReentrantSetObjectOwnPropertyHelper(v8::Isolate* isolate,
                                         v8::Local<v8::Object> obj,
                                         ByteStringView bsUTF8PropertyName,
                                         v8::Local<v8::Value> value);
bool ReentrantPutObjectPropertyHelper(v8::Isolate* isolate,
                                      v8::Local<v8::Object> obj,
                                      ByteStringView bsUTF8PropertyName,
                                      v8::Local<v8::Value> pPut);
void ReentrantDeleteObjectPropertyHelper(v8::Isolate* isolate,
                                         v8::Local<v8::Object> obj,
                                         ByteStringView bsUTF8PropertyName);

bool ReentrantPutArrayElementHelper(v8::Isolate* isolate,
                                    v8::Local<v8::Array> array,
                                    size_t index,
                                    v8::Local<v8::Value> value);
v8::Local<v8::Value> ReentrantGetArrayElementHelper(v8::Isolate* isolate,
                                                    v8::Local<v8::Array> array,
                                                    size_t index);
size_t GetArrayLengthHelper(v8::Local<v8::Array> array);

void ThrowExceptionHelper(v8::Isolate* isolate, ByteStringView str);
void ThrowExceptionHelper(v8::Isolate* isolate, WideStringView str);

}  // namespace fxv8

#endif  // FXJS_FXV8_H_
