// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/xml/cfx_xmldocument.h"
#include "core/fxcrt/xml/cfx_xmlelement.h"
#include "core/fxcrt/xml/cfx_xmlinstruction.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(CFXXMLDocumentTest, Root) {
  CFX_XMLDocument doc;
  EXPECT_TRUE(doc.GetRoot() != nullptr);
}

TEST(CFXXMLDocumentTest, CreateNode) {
  CFX_XMLDocument doc;
  auto* node = doc.CreateNode<CFX_XMLElement>(L"elem");

  ASSERT_EQ(CFX_XMLNode::Type::kElement, node->GetType());
  EXPECT_EQ(L"elem", node->GetName());
}

TEST(CFXXMLDocumentTest, AppendNodesFrom) {
  CFX_XMLDocument doc1;
  auto* elem1 = doc1.CreateNode<CFX_XMLElement>(L"elem1");
  doc1.GetRoot()->AppendLastChild(elem1);

  CFX_XMLDocument doc2;
  auto* elem2 = doc2.CreateNode<CFX_XMLElement>(L"elem2");

  doc1.AppendNodesFrom(&doc2);
  doc1.GetRoot()->AppendLastChild(elem2);

  EXPECT_EQ(L"elem1", ToXMLElement(doc1.GetRoot()->GetFirstChild())->GetName());
  EXPECT_EQ(L"elem2",
            ToXMLElement(doc1.GetRoot()->GetFirstChild()->GetNextSibling())
                ->GetName());
}
