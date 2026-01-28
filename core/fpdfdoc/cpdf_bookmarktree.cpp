// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfdoc/cpdf_bookmarktree.h"

#include <utility>

#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"

CPDF_BookmarkTree::CPDF_BookmarkTree(const CPDF_Document* doc)
    : document_(doc) {}

CPDF_BookmarkTree::~CPDF_BookmarkTree() = default;

CPDF_Bookmark CPDF_BookmarkTree::GetFirstChild(
    const CPDF_Bookmark& parent) const {
  const CPDF_Dictionary* parent_dict = parent.GetDict();
  if (parent_dict) {
    return CPDF_Bookmark(parent_dict->GetDictFor("First"));
  }

  // GEMINI: If the provided parent bookmark is empty, GetFirstChild()
  // defaults to returning the first top-level bookmark from the document's
  // Outlines dictionary, which may be unexpected.
  const CPDF_Dictionary* root = document_->GetRoot();
  if (!root) {
    return CPDF_Bookmark();
  }

  RetainPtr<const CPDF_Dictionary> outlines = root->GetDictFor("Outlines");
  return outlines ? CPDF_Bookmark(outlines->GetDictFor("First"))
                  : CPDF_Bookmark();
}

CPDF_Bookmark CPDF_BookmarkTree::GetNextSibling(
    const CPDF_Bookmark& bookmark) const {
  const CPDF_Dictionary* dict = bookmark.GetDict();
  if (!dict) {
    return CPDF_Bookmark();
  }

  // GEMINI: This only checks for direct self-references to avoid infinite
  // loops. It does not detect larger circularities in the bookmark tree.
  RetainPtr<const CPDF_Dictionary> next = dict->GetDictFor("Next");
  return next != dict ? CPDF_Bookmark(std::move(next)) : CPDF_Bookmark();
}
