// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cfxjse_runtimedata.h"

#include <utility>

#include "core/fxcrt/check_op.h"
#include "fxjs/cfxjs_engine.h"
#include "fxjs/fxv8.h"
#include "fxjs/xfa/cfxjse_isolatetracker.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-external.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-object.h"
#include "v8/include/v8-primitive.h"
#include "v8/include/v8-template.h"

CFXJSE_RuntimeData::CFXJSE_RuntimeData() = default;

CFXJSE_RuntimeData::~CFXJSE_RuntimeData() = default;

std::unique_ptr<CFXJSE_RuntimeData> CFXJSE_RuntimeData::Create(
    v8::Isolate* isolate) {
  std::unique_ptr<CFXJSE_RuntimeData> pRuntimeData(new CFXJSE_RuntimeData());
  CFXJSE_ScopeUtil_IsolateHandle scope(isolate);
  v8::Local<v8::FunctionTemplate> hFuncTemplate =
      v8::FunctionTemplate::New(isolate);

  v8::Local<v8::ObjectTemplate> global_template =
      hFuncTemplate->InstanceTemplate();
  global_template->Set(v8::Symbol::GetToStringTag(isolate),
                       fxv8::NewStringHelper(isolate, "global"));

  v8::Local<v8::Context> hContext =
      v8::Context::New(isolate, nullptr, global_template);

  DCHECK_EQ(hContext->Global()->InternalFieldCount(), 0);
  DCHECK_EQ(
      hContext->Global()->GetPrototype().As<v8::Object>()->InternalFieldCount(),
      0);

  hContext->SetSecurityToken(v8::External::New(isolate, isolate));
  pRuntimeData->root_context_global_template_.Reset(isolate, hFuncTemplate);
  pRuntimeData->root_context_.Reset(isolate, hContext);
  return pRuntimeData;
}

CFXJSE_RuntimeData* CFXJSE_RuntimeData::Get(v8::Isolate* isolate) {
  CFXJS_PerIsolateData::SetUp(isolate);
  CFXJS_PerIsolateData* pData = CFXJS_PerIsolateData::Get(isolate);
  if (!pData->GetExtension()) {
    pData->SetExtension(CFXJSE_RuntimeData::Create(isolate));
  }
  return static_cast<CFXJSE_RuntimeData*>(pData->GetExtension());
}

v8::Local<v8::Context> CFXJSE_RuntimeData::GetRootContext(
    v8::Isolate* isolate) {
  return v8::Local<v8::Context>::New(isolate, root_context_);
}
