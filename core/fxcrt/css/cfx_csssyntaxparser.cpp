// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcrt/css/cfx_csssyntaxparser.h"

#include "core/fxcrt/css/cfx_cssdata.h"
#include "core/fxcrt/css/cfx_cssdeclaration.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_extension.h"

namespace {

bool IsSelectorStart(wchar_t wch) {
  return wch == '.' || wch == '#' || wch == '*' ||
         (isascii(wch) && isalpha(wch));
}

}  // namespace

CFX_CSSSyntaxParser::CFX_CSSSyntaxParser(WideStringView str) : m_Input(str) {}

CFX_CSSSyntaxParser::~CFX_CSSSyntaxParser() = default;

void CFX_CSSSyntaxParser::SetParseOnlyDeclarations() {
  mode_ = Mode::kPropertyName;
}

CFX_CSSSyntaxParser::Status CFX_CSSSyntaxParser::DoSyntaxParse() {
  m_Output.Clear();
  if (has_error_) {
    return Status::kError;
  }

  while (!m_Input.IsEOF()) {
    wchar_t wch = m_Input.GetChar();
    switch (mode_) {
      case Mode::kRuleSet:
        switch (wch) {
          case '}':
            has_error_ = true;
            return Status::kError;
          case '/':
            if (m_Input.GetNextChar() == '*') {
              SaveMode(Mode::kRuleSet);
              mode_ = Mode::kComment;
              break;
            }
            [[fallthrough]];
          default:
            if (wch <= ' ') {
              m_Input.MoveNext();
            } else if (IsSelectorStart(wch)) {
              mode_ = Mode::kSelector;
              return Status::kStyleRule;
            } else {
              has_error_ = true;
              return Status::kError;
            }
            break;
        }
        break;
      case Mode::kSelector:
        switch (wch) {
          case ',':
            m_Input.MoveNext();
            if (!m_Output.IsEmpty())
              return Status::kSelector;
            break;
          case '{':
            if (!m_Output.IsEmpty())
              return Status::kSelector;
            m_Input.MoveNext();
            SaveMode(Mode::kRuleSet);  // Back to validate ruleset again.
            mode_ = Mode::kPropertyName;
            return Status::kDeclOpen;
          case '/':
            if (m_Input.GetNextChar() == '*') {
              SaveMode(Mode::kSelector);
              mode_ = Mode::kComment;
              if (!m_Output.IsEmpty())
                return Status::kSelector;
              break;
            }
            [[fallthrough]];
          default:
            m_Output.AppendCharIfNotLeadingBlank(wch);
            m_Input.MoveNext();
            break;
        }
        break;
      case Mode::kPropertyName:
        switch (wch) {
          case ':':
            m_Input.MoveNext();
            mode_ = Mode::kPropertyValue;
            return Status::kPropertyName;
          case '}':
            m_Input.MoveNext();
            if (!RestoreMode())
              return Status::kError;

            return Status::kDeclClose;
          case '/':
            if (m_Input.GetNextChar() == '*') {
              SaveMode(Mode::kPropertyName);
              mode_ = Mode::kComment;
              if (!m_Output.IsEmpty())
                return Status::kPropertyName;
              break;
            }
            [[fallthrough]];
          default:
            m_Output.AppendCharIfNotLeadingBlank(wch);
            m_Input.MoveNext();
            break;
        }
        break;
      case Mode::kPropertyValue:
        switch (wch) {
          case ';':
            m_Input.MoveNext();
            [[fallthrough]];
          case '}':
            mode_ = Mode::kPropertyName;
            return Status::kPropertyValue;
          case '/':
            if (m_Input.GetNextChar() == '*') {
              SaveMode(Mode::kPropertyValue);
              mode_ = Mode::kComment;
              if (!m_Output.IsEmpty())
                return Status::kPropertyValue;
              break;
            }
            [[fallthrough]];
          default:
            m_Output.AppendCharIfNotLeadingBlank(wch);
            m_Input.MoveNext();
            break;
        }
        break;
      case Mode::kComment:
        if (wch == '*' && m_Input.GetNextChar() == '/') {
          if (!RestoreMode())
            return Status::kError;
          m_Input.MoveNext();
        }
        m_Input.MoveNext();
        break;
    }
  }
  if (mode_ == Mode::kPropertyValue && !m_Output.IsEmpty()) {
    return Status::kPropertyValue;
  }

  return Status::kEOS;
}

void CFX_CSSSyntaxParser::SaveMode(Mode mode) {
  m_ModeStack.push(mode);
}

bool CFX_CSSSyntaxParser::RestoreMode() {
  if (m_ModeStack.empty()) {
    has_error_ = true;
    return false;
  }
  mode_ = m_ModeStack.top();
  m_ModeStack.pop();
  return true;
}

WideStringView CFX_CSSSyntaxParser::GetCurrentString() const {
  return m_Output.GetTrailingBlankTrimmedString();
}
