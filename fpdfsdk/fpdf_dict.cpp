// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdf_dict.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "public/fpdfview.h"

extern "C" {
FPDF_EXPORT FPDF_DICTIONARY FPDF_CALLCONV
FPDF_GetPageDictionary(FPDF_DOCUMENT doc, int page_index) {
  CPDF_Document* document = CPDFDocumentFromFPDFDocument(doc);
  if (!document) {
    return nullptr;
  }
  return reinterpret_cast<FPDF_DICTIONARY>(const_cast<CPDF_Dictionary*>(
      document->GetPageDictionary(page_index).Get()));
}

FPDF_EXPORT FPDF_RESULT_STRING FPDF_CALLCONV
FPDF_DictionaryGetString(FPDF_DICTIONARY dictionary, FPDF_BYTESTRING key) {
  const CPDF_Dictionary* dict = CPDFDictFromFPDFDict(dictionary);
  if (!dict || !dict->KeyExist(key)) {
    return {false, nullptr};
  }
  return {true, reinterpret_cast<FPDF_STRING>(
                    dict->GetStringFor(key).Get()->GetString().c_str())};
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
  return reinterpret_cast<FPDF_DICTIONARY>(dict->GetDictFor(key).Get());
}

FPDF_EXPORT FS_RECTF FPDF_CALLCONV
FPDF_DictionaryGetRect(FPDF_DICTIONARY dictionary, FPDF_BYTESTRING key) {
  const CPDF_Dictionary* dict = CPDFDictFromFPDFDict(dictionary);
  if (!dict) {
    return {0.0f, 0.0f, 0.0f, 0.0f};
  }
  CFX_FloatRect rect = dict->GetRectFor(key);
  return {rect.left, rect.top, rect.right, rect.bottom};
}

FPDF_EXPORT FS_MATRIX FPDF_CALLCONV
FPDF_DictionaryGetMatrix(FPDF_DICTIONARY dictionary, FPDF_BYTESTRING key) {
  const CPDF_Dictionary* dict = CPDFDictFromFPDFDict(dictionary);
  if (!dict) {
    return {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
  }
  CFX_Matrix matrix = dict->GetMatrixFor(key);
  return {matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f};
}
}
