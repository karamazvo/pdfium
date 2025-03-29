// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/cxfa_readynodeiterator.h"

#include "xfa/fxfa/parser/cxfa_node.h"

CXFA_ReadyNodeIterator::CXFA_ReadyNodeIterator(CXFA_Node* pTravelRoot)
    : m_ContentIterator(pTravelRoot) {}

CXFA_ReadyNodeIterator::~CXFA_ReadyNodeIterator() = default;

CXFA_Node* CXFA_ReadyNodeIterator::MoveToNext() {
  CXFA_Node* pItem = cur_node_ ? m_ContentIterator.MoveToNext()
                               : m_ContentIterator.GetCurrent();
  while (pItem) {
    cur_node_ = pItem->IsWidgetReady() ? pItem : nullptr;
    if (cur_node_) {
      return cur_node_;
    }
    pItem = m_ContentIterator.MoveToNext();
  }
  return nullptr;
}

void CXFA_ReadyNodeIterator::SkipTree() {
  m_ContentIterator.SkipChildrenAndMoveToNext();
  cur_node_ = nullptr;
}
