// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFDOC_CPDF_NUMBERTREE_H_
#define CORE_FPDFDOC_CPDF_NUMBERTREE_H_

#include "core/fxcrt/retain_ptr.h"

class CPDF_Dictionary;
class CPDF_Object;

// Represents a number tree that allows for sub-linear lookups of tree nodes.
// See ISO 32000-1:2008 spec, section 7.9.7.
class CPDF_NumberTree {
 public:
  struct KeyValue {
    KeyValue();
    KeyValue(int key, RetainPtr<const CPDF_Object> value);
    KeyValue(KeyValue&) = delete;
    KeyValue& operator=(KeyValue&) = delete;
    KeyValue(KeyValue&&) noexcept;
    KeyValue& operator=(KeyValue&&) noexcept;
    ~KeyValue();

    int key;
    RetainPtr<const CPDF_Object> value;
  };

  explicit CPDF_NumberTree(RetainPtr<const CPDF_Dictionary> root);
  ~CPDF_NumberTree();

  // Finds the object in the number tree whose key is `num`. Returns nullptr in
  // there is no `num` key in the tree.
  RetainPtr<const CPDF_Object> LookupValue(int num) const;

  // Finds the object in the number tree with the largest key, such that
  // `num` >= key. Returns true and write the key/value pair to the out
  // parameter, if such a key exists, or false otherwise otherwise.
  // Note that this is similar to, but not exactly the same as
  // std::lower_bound().
  // TODO(crbug.com/42270941): Return std::optional<KeyValue>.
  bool GetLowerBound(int num, KeyValue& result) const;

 protected:
  RetainPtr<const CPDF_Dictionary> const root_;
};

#endif  // CORE_FPDFDOC_CPDF_NUMBERTREE_H_
