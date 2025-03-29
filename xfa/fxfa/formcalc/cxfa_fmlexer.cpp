// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/formcalc/cxfa_fmlexer.h"

#include <algorithm>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/stl_util.h"

namespace {

bool IsFormCalcCharacter(wchar_t c) {
  return (c >= 0x09 && c <= 0x0D) || (c >= 0x20 && c <= 0xd7FF) ||
         (c >= 0xE000 && c <= 0xFFFD);
}

bool IsIdentifierCharacter(wchar_t c) {
  return FXSYS_iswalnum(c) || c == 0x005F ||  // '_'
         c == 0x0024;                         // '$'
}

bool IsInitialIdentifierCharacter(wchar_t c) {
  return FXSYS_iswalpha(c) || c == 0x005F ||  // '_'
         c == 0x0024 ||                       // '$'
         c == 0x0021;                         // '!'
}

bool IsWhitespaceCharacter(wchar_t c) {
  return c == 0x0009 ||  // Horizontal tab
         c == 0x000B ||  // Vertical tab
         c == 0x000C ||  // Form feed
         c == 0x0020;    // Space
}

struct XFA_FMKeyword {
  XFA_FM_TOKEN m_type;
  const char* m_keyword;  // Raw, POD struct.
};

const XFA_FMKeyword kKeyWords[] = {
    {TOKdo, "do"},
    {TOKkseq, "eq"},
    {TOKksge, "ge"},
    {TOKksgt, "gt"},
    {TOKif, "if"},
    {TOKin, "in"},
    {TOKksle, "le"},
    {TOKkslt, "lt"},
    {TOKksne, "ne"},
    {TOKksor, "or"},
    {TOKnull, "null"},
    {TOKbreak, "break"},
    {TOKksand, "and"},
    {TOKend, "end"},
    {TOKeof, "eof"},
    {TOKfor, "for"},
    {TOKnan, "nan"},
    {TOKksnot, "not"},
    {TOKvar, "var"},
    {TOKthen, "then"},
    {TOKelse, "else"},
    {TOKexit, "exit"},
    {TOKdownto, "downto"},
    {TOKreturn, "return"},
    {TOKinfinity, "infinity"},
    {TOKendwhile, "endwhile"},
    {TOKforeach, "foreach"},
    {TOKendfunc, "endfunc"},
    {TOKelseif, "elseif"},
    {TOKwhile, "while"},
    {TOKendfor, "endfor"},
    {TOKthrow, "throw"},
    {TOKstep, "step"},
    {TOKupto, "upto"},
    {TOKcontinue, "continue"},
    {TOKfunc, "func"},
    {TOKendif, "endif"},
};

XFA_FM_TOKEN TokenizeIdentifier(WideStringView str) {
  const XFA_FMKeyword* result =
      std::find_if(std::begin(kKeyWords), std::end(kKeyWords),
                   [str](const XFA_FMKeyword& iter) {
                     return str.EqualsASCII(iter.m_keyword);
                   });
  if (result != std::end(kKeyWords) && str.EqualsASCII(result->m_keyword)) {
    return result->m_type;
  }
  return TOKidentifier;
}

}  // namespace

CXFA_FMLexer::Token::Token() = default;

CXFA_FMLexer::Token::Token(XFA_FM_TOKEN token) : m_type(token) {}

CXFA_FMLexer::Token::Token(XFA_FM_TOKEN token, WideStringView str)
    : m_type(token), m_string(str) {}

CXFA_FMLexer::Token::Token(const Token& that) = default;

CXFA_FMLexer::Token::~Token() = default;

CXFA_FMLexer::CXFA_FMLexer(WideStringView wsFormCalc)
    : m_spInput(wsFormCalc.span()) {}

CXFA_FMLexer::~CXFA_FMLexer() = default;

CXFA_FMLexer::Token CXFA_FMLexer::NextToken() {
  if (lexer_error_) {
    return Token();
  }

  while (!IsComplete() && m_spInput[cursor_]) {
    if (!IsFormCalcCharacter(m_spInput[cursor_])) {
      RaiseError();
      return Token();
    }

    switch (m_spInput[cursor_]) {
      case '\n':
        ++cursor_;
        break;
      case '\r':
        ++cursor_;
        break;
      case ';':
        AdvanceForComment();
        break;
      case '"':
        return AdvanceForString();
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        return AdvanceForNumber();
      case '=':
        ++cursor_;
        if (cursor_ >= m_spInput.size()) {
          return Token(TOKassign);
        }

        if (!IsFormCalcCharacter(m_spInput[cursor_])) {
          RaiseError();
          return Token();
        }
        if (m_spInput[cursor_] == '=') {
          ++cursor_;
          return Token(TOKeq);
        }
        return Token(TOKassign);
      case '<':
        ++cursor_;
        if (cursor_ >= m_spInput.size()) {
          return Token(TOKlt);
        }

        if (!IsFormCalcCharacter(m_spInput[cursor_])) {
          RaiseError();
          return Token();
        }
        if (m_spInput[cursor_] == '=') {
          ++cursor_;
          return Token(TOKle);
        }
        if (m_spInput[cursor_] == '>') {
          ++cursor_;
          return Token(TOKne);
        }
        return Token(TOKlt);
      case '>':
        ++cursor_;
        if (cursor_ >= m_spInput.size()) {
          return Token(TOKgt);
        }

        if (!IsFormCalcCharacter(m_spInput[cursor_])) {
          RaiseError();
          return Token();
        }
        if (m_spInput[cursor_] == '=') {
          ++cursor_;
          return Token(TOKge);
        }
        return Token(TOKgt);
      case ',':
        ++cursor_;
        return Token(TOKcomma);
      case '(':
        ++cursor_;
        return Token(TOKlparen);
      case ')':
        ++cursor_;
        return Token(TOKrparen);
      case '[':
        ++cursor_;
        return Token(TOKlbracket);
      case ']':
        ++cursor_;
        return Token(TOKrbracket);
      case '&':
        ++cursor_;
        return Token(TOKand);
      case '|':
        ++cursor_;
        return Token(TOKor);
      case '+':
        ++cursor_;
        return Token(TOKplus);
      case '-':
        ++cursor_;
        return Token(TOKminus);
      case '*':
        ++cursor_;
        return Token(TOKmul);
      case '/': {
        ++cursor_;
        if (cursor_ >= m_spInput.size()) {
          return Token(TOKdiv);
        }

        if (!IsFormCalcCharacter(m_spInput[cursor_])) {
          RaiseError();
          return Token();
        }
        if (m_spInput[cursor_] != '/') {
          return Token(TOKdiv);
        }

        AdvanceForComment();
        break;
      }
      case '.':
        ++cursor_;
        if (cursor_ >= m_spInput.size()) {
          return Token(TOKdot);
        }

        if (!IsFormCalcCharacter(m_spInput[cursor_])) {
          RaiseError();
          return Token();
        }

        if (m_spInput[cursor_] == '.') {
          ++cursor_;
          return Token(TOKdotdot);
        }
        if (m_spInput[cursor_] == '*') {
          ++cursor_;
          return Token(TOKdotstar);
        }
        if (m_spInput[cursor_] == '#') {
          ++cursor_;
          return Token(TOKdotscream);
        }
        if (FXSYS_IsDecimalDigit(m_spInput[cursor_])) {
          --cursor_;
          return AdvanceForNumber();
        }
        return Token(TOKdot);
      default:
        if (IsWhitespaceCharacter(m_spInput[cursor_])) {
          ++cursor_;
          break;
        }
        if (!IsInitialIdentifierCharacter(m_spInput[cursor_])) {
          RaiseError();
          return Token();
        }
        return AdvanceForIdentifier();
    }
  }
  return Token(TOKeof);
}

CXFA_FMLexer::Token CXFA_FMLexer::AdvanceForNumber() {
  // This will set end to the character after the end of the number.
  size_t used_length = 0;
  if (cursor_ < m_spInput.size()) {
    FXSYS_wcstof(WideStringView(m_spInput.subspan(cursor_)), &used_length);
  }
  size_t end = cursor_ + used_length;
  if (used_length == 0 ||
      (end < m_spInput.size() && FXSYS_iswalpha(m_spInput[end]))) {
    RaiseError();
    return Token();
  }
  WideStringView str(m_spInput.subspan(cursor_, end - cursor_));
  cursor_ = end;
  return Token(TOKnumber, str);
}

CXFA_FMLexer::Token CXFA_FMLexer::AdvanceForString() {
  size_t start = cursor_;
  ++cursor_;
  while (!IsComplete() && m_spInput[cursor_]) {
    if (!IsFormCalcCharacter(m_spInput[cursor_])) {
      break;
    }

    if (m_spInput[cursor_] == '"') {
      // Check for escaped "s, i.e. "".
      ++cursor_;
      // If the end of the input has been reached it was not escaped.
      if (cursor_ >= m_spInput.size()) {
        return Token(TOKstring,
                     WideStringView(m_spInput.subspan(start, cursor_ - start)));
      }
      // If the next character is not a " then the end of the string has been
      // found.
      if (m_spInput[cursor_] != '"') {
        if (!IsFormCalcCharacter(m_spInput[cursor_])) {
          break;
        }

        return Token(TOKstring,
                     WideStringView(m_spInput.subspan(start, cursor_ - start)));
      }
    }
    ++cursor_;
  }

  // Didn't find the end of the string.
  RaiseError();
  return Token();
}

CXFA_FMLexer::Token CXFA_FMLexer::AdvanceForIdentifier() {
  size_t start = cursor_;
  ++cursor_;
  while (!IsComplete() && m_spInput[cursor_]) {
    if (!IsFormCalcCharacter(m_spInput[cursor_])) {
      RaiseError();
      return Token();
    }
    if (!IsIdentifierCharacter(m_spInput[cursor_])) {
      break;
    }

    ++cursor_;
  }

  WideStringView str(m_spInput.subspan(start, cursor_ - start));
  return Token(TokenizeIdentifier(str), str);
}

void CXFA_FMLexer::AdvanceForComment() {
  ++cursor_;
  while (!IsComplete() && m_spInput[cursor_]) {
    if (!IsFormCalcCharacter(m_spInput[cursor_])) {
      RaiseError();
      return;
    }
    if (m_spInput[cursor_] == L'\r') {
      ++cursor_;
      return;
    }
    if (m_spInput[cursor_] == L'\n') {
      ++cursor_;
      return;
    }
    ++cursor_;
  }
}
