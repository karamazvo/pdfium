// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FPDFAPI_PARSER_CPDF_OBJECT_STREAM_H_
#define CORE_FPDFAPI_PARSER_CPDF_OBJECT_STREAM_H_

#include <memory>
#include <vector>

#include "core/fpdfapi/parser/cpdf_object.h"
#include "core/fxcrt/retain_ptr.h"

class CPDF_IndirectObjectHolder;
class CPDF_Stream;
class CPDF_StreamAcc;
class CPDF_SyntaxParser;

// Implementation of logic of PDF "Object Streams".
// See ISO 32000-1:2008 spec, section 7.5.7.
class CPDF_ObjectStream {
 public:
  struct ObjectInfo {
    ObjectInfo(uint32_t obj_num, uint32_t obj_offset)
        : obj_num(obj_num), obj_offset(obj_offset) {}

    bool operator==(const ObjectInfo& that) const {
      return obj_num == that.obj_num && obj_offset == that.obj_offset;
    }

    uint32_t obj_num;
    uint32_t obj_offset;
  };

  static std::unique_ptr<CPDF_ObjectStream> Create(
      RetainPtr<const CPDF_Stream> stream,
      CPDF_IndirectObjectHolder* pObjList);

  ~CPDF_ObjectStream();

  RetainPtr<CPDF_Object> ParseObject(uint32_t obj_number,
                                     uint32_t archive_obj_index) const;
  const std::vector<ObjectInfo>& object_info() const { return object_info_; }

 private:
  CPDF_ObjectStream(RetainPtr<const CPDF_Stream> stream,
                    CPDF_IndirectObjectHolder* pObjList);

  void Init(const CPDF_Stream* stream, CPDF_IndirectObjectHolder* pObjList);
  RetainPtr<CPDF_Object> ParseObjectAtOffset(uint32_t object_offset) const;

  // Must outlive `syntax_parser_`.
  RetainPtr<CPDF_StreamAcc> const stream_acc_;
  std::unique_ptr<CPDF_SyntaxParser> syntax_parser_;
  int first_object_offset_ = 0;
  std::vector<ObjectInfo> object_info_;
};

#endif  // CORE_FPDFAPI_PARSER_CPDF_OBJECT_STREAM_H_
