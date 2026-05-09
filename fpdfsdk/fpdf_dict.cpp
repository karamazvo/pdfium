// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdf_dict.h"

#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "public/fpdf_edit.h"
#include "public/fpdfview.h"

extern "C" {
FPDF_EXPORT FPDF_DICTIONARY FPDF_CALLCONV
FPDF_GetPageDictionary(FPDF_DOCUMENT doc, int page_index) {
  CPDF_Document* document = CPDFDocumentFromFPDFDocument(doc);
  if (!document) {
    return nullptr;
  }
  return FPDFDictFromCPDFDict(document->GetPageDictionary(page_index).Get());
}

FPDF_EXPORT FPDF_DICTIONARY FPDF_CALLCONV
FPDF_GetFontDictionary(FPDF_FONT font) {
  CPDF_Font* cpdf_font = CPDFFontFromFPDFFont(font);
  if (!cpdf_font) {
    return nullptr;
  }
  return FPDFDictFromCPDFDict(cpdf_font->GetFontDict());
}

FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDF_DictionaryGetString(FPDF_DICTIONARY dictionary,
                         FPDF_BYTESTRING key,
                         void* buffer,
                         unsigned long buflen) {
  const CPDF_Dictionary* pDict = CPDFDictFromFPDFDict(dictionary);
  if (!pDict || !key) {
    return 0;
  }
  // SAFETY: required from caller.
  return Utf16EncodeMaybeCopyAndReturnLength(
      pDict->GetUnicodeTextFor(key),
      UNSAFE_BUFFERS(SpanFromFPDFApiArgs(buffer, buflen)));
}

FPDF_EXPORT FPDF_RESULT_INT FPDF_CALLCONV
FPDF_DictionaryGetInt(FPDF_DICTIONARY dictionary, FPDF_BYTESTRING key) {
  const CPDF_Dictionary* dict = CPDFDictFromFPDFDict(dictionary);
  if (!dict || !dict->KeyExist(key)) {
    return {false, 0};
  }
  return {true, dict->GetIntegerFor(key)};
}

FPDF_EXPORT FPDF_RESULT_BOOL FPDF_CALLCONV
FPDF_DictionaryGetBool(FPDF_DICTIONARY dictionary, FPDF_BYTESTRING key) {
  const CPDF_Dictionary* dict = CPDFDictFromFPDFDict(dictionary);
  if (!dict || !dict->KeyExist(key)) {
    return {false, false};
  }
  return {true, dict->GetBooleanFor(key, false)};
}

FPDF_EXPORT FPDF_RESULT_FLOAT FPDF_CALLCONV
FPDF_DictionaryGetFloat(FPDF_DICTIONARY dictionary, FPDF_BYTESTRING key) {
  const CPDF_Dictionary* dict = CPDFDictFromFPDFDict(dictionary);
  if (!dict || !dict->KeyExist(key)) {
    return {false, 0.0f};
  }
  return {true, dict->GetFloatFor(key)};
}

FPDF_EXPORT FPDF_DICTIONARY FPDF_CALLCONV
FPDF_DictionaryGetDict(FPDF_DICTIONARY dictionary, FPDF_BYTESTRING key) {
  const CPDF_Dictionary* dict = CPDFDictFromFPDFDict(dictionary);
  if (!dict) {
    return nullptr;
  }
  return FPDFDictFromCPDFDict(dict->GetDictFor(key).Get());
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDF_DictionaryGetRect(FPDF_DICTIONARY dictionary,
                       FPDF_BYTESTRING key,
                       FS_RECTF* rect) {
  const CPDF_Dictionary* dict = CPDFDictFromFPDFDict(dictionary);
  if (!dict) {
    return false;
  }
  CFX_FloatRect r = dict->GetRectFor(key);
  *rect = {r.left, r.top, r.right, r.bottom};
  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDF_DictionaryGetMatrix(FPDF_DICTIONARY dictionary,
                         FPDF_BYTESTRING key,
                         FS_MATRIX* matrix) {
  const CPDF_Dictionary* dict = CPDFDictFromFPDFDict(dictionary);
  if (!dict) {
    return false;
  }
  CFX_Matrix m = dict->GetMatrixFor(key);
  *matrix = {m.a, m.b, m.c, m.d, m.e, m.f};
  return true;
}
}
