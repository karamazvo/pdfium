// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFTEXT_CPDF_TEXTPAGE_H_
#define CORE_FPDFTEXT_CPDF_TEXTPAGE_H_

#include <stdint.h>

#include <deque>
#include <functional>
#include <optional>
#include <vector>

#include "core/fpdfapi/page/cpdf_pageobjectholder.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_memory_wrappers.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxcrt/widestring.h"
#include "core/fxcrt/widetext_buffer.h"

class CPDF_FormObject;
class CPDF_Page;
class CPDF_TextObject;

class CPDF_TextPage {
 public:
  enum class CharType : uint8_t {
    kNormal,
    kGenerated,  // All generated CharInfo have CPDF_Font::kInvalidCharCode
    kNotUnicode,
    kHyphen,
    kPiece, // Probably just get rid of this?
    kPieceContinue,
  };

  struct Range { size_t begin; size_t end; };

  class CharInfo { // GlyphInfo, really. The unicode text needs to be stored separately with a cluster mapping.
   public:
    CharInfo();
    CharInfo(CharType char_type,
             uint32_t char_code,
             CFX_PointF origin,
             CFX_FloatRect char_box,
             CFX_Matrix matrix,
             Range text_index,
             CPDF_TextObject* text_object);
    CharInfo(const CharInfo&);
    ~CharInfo();

    CharType char_type() const { return char_type_; }
    void set_char_type(CharType char_type) { char_type_ = char_type; }

    uint32_t char_code() const { return char_code_; }

    Range text_index() const { return text_index_; }
    //int32_t unicode() const { return unicode_; } // Now a view, but cannot be reported from here.
    //void set_unicode(int32_t unicode) { unicode_ = unicode; } // Not really a thing anymore.

    const CFX_PointF& origin() const { return origin_; }

    const CFX_FloatRect& char_box() const { return char_box_; }
    const CFX_FloatRect& loose_char_box() const { return loose_char_box_; }

    const CFX_Matrix& matrix() const { return matrix_; }

    const CPDF_TextObject* text_object() const { return text_object_; }
    CPDF_TextObject* text_object() { return text_object_; }

   private:
    CharType char_type_ = CharType::kNormal;
    uint32_t char_code_ = 0;
    CFX_PointF origin_;
    CFX_FloatRect char_box_;
    CFX_FloatRect loose_char_box_;
    CFX_Matrix matrix_;
    Range text_index_; // Index of first code point.
    UnownedPtr<CPDF_TextObject> text_object_;
  };

  CPDF_TextPage(const CPDF_Page* pPage, bool rtl);
  ~CPDF_TextPage();

  int CharIndexFromTextIndex(int text_index) const;
  int TextIndexFromCharIndex(int char_index) const;
  size_t size() const { return char_list_.size(); }
  int CountChars() const;

  // These methods CHECK() to make sure |index| is within bounds.
  const CharInfo& GetCharInfo(size_t index) const;
  CharInfo& GetCharInfo(size_t index);
  float GetCharFontSize(size_t index) const;
  CFX_FloatRect GetCharLooseBounds(size_t index) const;

  std::vector<CFX_FloatRect> GetRectArray(int start, int count) const;
  int GetIndexAtPos(const CFX_PointF& point, const CFX_SizeF& tolerance) const;
  WideString GetTextByRect(const CFX_FloatRect& rect) const;
  WideString GetTextByObject(const CPDF_TextObject* pTextObj) const;

  // Returns string with the text from |text_buf_| that are covered by the input
  // range. |start| and |count| are in terms of the |char_indices_|, so the
  // range will be converted into appropriate indices.
  WideString GetPageText(int start, int count) const;
  WideString GetAllPageText() const { return GetPageText(0, CountChars()); }

  int CountRects(int start, int nCount);
  bool GetRect(int rectIndex, CFX_FloatRect* pRect) const;

 private:
  enum class TextOrientation {
    kUnknown,
    kHorizontal,
    kVertical,
  };

  enum class GenerateCharacter {
    kNone,
    kSpace,
    kLineBreak,
    kHyphen,
  };

  enum class MarkedContentState { kPass = 0, kDone, kDelay };

  struct TransformedTextObject {
    TransformedTextObject();
    TransformedTextObject(const TransformedTextObject& that);
    ~TransformedTextObject();

    UnownedPtr<CPDF_TextObject> text_obj_;
    CFX_Matrix form_matrix_;
  };

  void Init();
  bool IsHyphen(int32_t unicode) const; // More like "ShouldMarkAPreviousHyphenAsGeneratedBeforeAppending"
  void ProcessObject();
  void ProcessFormObject(CPDF_FormObject* pFormObj,
                         const CFX_Matrix& form_matrix);
  void ProcessTextObject(const TransformedTextObject& obj);
  void ProcessTextObject(CPDF_TextObject* pTextObj,
                         const CFX_Matrix& form_matrix,
                         const CPDF_PageObjectHolder* pObjList,
                         CPDF_PageObjectHolder::const_iterator ObjPos);
  GenerateCharacter ProcessInsertObject(const CPDF_TextObject* pObj,
                                        const CFX_Matrix& form_matrix);
  // Returns whether to continue or not.
  bool ProcessGenerateCharacter(GenerateCharacter type,
                                const CPDF_TextObject* text_object,
                                const CFX_Matrix& form_matrix);
  void ProcessTextObjectItems(CPDF_TextObject* text_object,
                              const CFX_Matrix& form_matrix,
                              const CFX_Matrix& matrix);
  const CharInfo* GetPrevCharInfo() const;
  std::optional<CharInfo> GenerateCharInfo(Range text_index,
                                           const CFX_Matrix& form_matrix);
  bool IsSameAsPreTextObject(CPDF_TextObject* pTextObj,
                             const CPDF_PageObjectHolder* pObjList,
                             CPDF_PageObjectHolder::const_iterator iter) const;
  bool IsSameTextObject(CPDF_TextObject* pTextObj1,
                        CPDF_TextObject* pTextObj2) const;
  void CloseTempLine();
  MarkedContentState PreMarkedContent(const TransformedTextObject& obj);
  void ProcessMarkedContent(const TransformedTextObject& obj);
  void FindPreviousTextObject();
  void AddCharInfoByLRDirection(int32_t unicode, const CharInfo& info);
  void AddCharInfoByRLDirection(int32_t unicode, const CharInfo& info);
  TextOrientation GetTextObjectWritingMode(
      const CPDF_TextObject* pTextObj) const;
  TextOrientation FindTextlineFlowOrientation() const;
  void AppendGeneratedCharacter(WideStringView unicode,
                                const CFX_Matrix& form_matrix,
                                bool use_temp_buffer);
  void SwapTempTextBuf(size_t iCharListStartAppend, size_t iBufStartAppend);
  WideString GetTextByPredicate(
      const std::function<bool(const CharInfo&)>& predicate) const;

  UnownedPtr<const CPDF_Page> const page_;
  // These are append only deques, just trying to avoid copying.
  // If the number of CharInfo or rough size of text is known before hand, that would be much faster.
  // TODO: temp_ means "for the current line"
  std::deque<CharInfo> char_list_;
  std::deque<CharInfo> temp_char_list_;
  std::deque<size_t> char_index_for_text_index_;  // These are a bit awkward since there may be multiple.
  std::deque<size_t> temp_char_index_for_text_index_; // Before there weren't because the first rect in ActualText got everything.
  WideTextBuffer text_buf_;
  WideTextBuffer temp_text_buf_;

  UnownedPtr<const CPDF_TextObject> prev_text_obj_;
  CFX_Matrix prev_matrix_;
  const bool rtl_;
  const CFX_Matrix display_matrix_;
  std::vector<CFX_FloatRect> sel_rects_;
  std::vector<TransformedTextObject> text_objects_;
  TextOrientation textline_dir_ = TextOrientation::kUnknown;
  CFX_FloatRect curline_rect_;
};

#endif  // CORE_FPDFTEXT_CPDF_TEXTPAGE_H_
