// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_CRT_CFGAS_STRINGFORMATTER_H_
#define XFA_FGAS_CRT_CFGAS_STRINGFORMATTER_H_

#include <vector>

#include "core/fxcrt/raw_span.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/widestring.h"
#include "xfa/fgas/crt/locale_iface.h"

class CFX_DateTime;
class LocaleMgrIface;

[[nodiscard]] bool FX_DateFromCanonical(pdfium::span<const wchar_t> wsTime,
                                        CFX_DateTime* datetime);
[[nodiscard]] bool FX_TimeFromCanonical(const LocaleIface* pLocale,
                                        pdfium::span<const wchar_t> wsTime,
                                        CFX_DateTime* datetime);

class CFGAS_StringFormatter {
 public:
  enum class Category {
    kUnknown,
    kDate,
    kTime,
    kDateTime,
    kNum,
    kText,
    kZero,
    kNull,
  };

  enum class DateTimeType {
    kUnknown,
    kDate,
    kTime,
    kDateTime,
    kTimeDate,
  };

  explicit CFGAS_StringFormatter(const WideString& wsPattern);
  ~CFGAS_StringFormatter();

  static std::vector<WideString> SplitOnBars(const WideString& wsFormatString);

  Category GetCategory() const;

  [[nodiscard]] bool ParseText(const WideString& wsSrcText,
                               WideString* wsValue) const;
  [[nodiscard]] bool ParseNum(LocaleMgrIface* pLocaleMgr,
                              const WideString& wsSrcNum,
                              WideString* wsValue) const;
  [[nodiscard]] bool ParseDateTime(LocaleMgrIface* pLocaleMgr,
                                   const WideString& wsSrcDateTime,
                                   DateTimeType eDateTimeType,
                                   CFX_DateTime* dtValue) const;
  [[nodiscard]] bool ParseZero(const WideString& wsSrcText) const;
  [[nodiscard]] bool ParseNull(const WideString& wsSrcText) const;

  [[nodiscard]] bool FormatText(const WideString& wsSrcText,
                                WideString* wsOutput) const;
  [[nodiscard]] bool FormatNum(LocaleMgrIface* pLocaleMgr,
                               const WideString& wsSrcNum,
                               WideString* wsOutput) const;
  [[nodiscard]] bool FormatDateTime(LocaleMgrIface* pLocaleMgr,
                                    const WideString& wsSrcDateTime,
                                    DateTimeType eDateTimeType,
                                    WideString* wsOutput) const;
  [[nodiscard]] bool FormatZero(WideString* wsOutput) const;
  [[nodiscard]] bool FormatNull(WideString* wsOutput) const;

 private:
  WideString GetTextFormat(WideStringView wsCategory) const;
  LocaleIface* GetNumericFormat(LocaleMgrIface* pLocaleMgr,
                                size_t* iDotIndex,
                                uint32_t* dwStyle,
                                WideString* wsPurgePattern) const;
  DateTimeType GetDateTimeFormat(LocaleMgrIface* pLocaleMgr,
                                 LocaleIface** pLocale,
                                 WideString* wsDatePattern,
                                 WideString* wsTimePattern) const;

  // keep pattern string alive.
  const WideString m_wsPattern;

  // span into `m_wsPattern`.
  const pdfium::raw_span<const wchar_t> m_spPattern;
};

#endif  // XFA_FGAS_CRT_CFGAS_STRINGFORMATTER_H_
