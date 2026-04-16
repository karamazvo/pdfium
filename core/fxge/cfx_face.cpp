// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_face.h"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/numerics/clamped_math.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "core/fxcrt/numerics/safe_math.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/cfx_fontmgr.h"
#include "core/fxge/cfx_gemodule.h"
#include "core/fxge/cfx_glyphbitmap.h"
#include "core/fxge/cfx_glyphcache.h"
#include "core/fxge/cfx_path.h"
#include "core/fxge/cfx_substfont.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"
#include "core/fxge/fx_font.h"
#include "core/fxge/fx_fontencoding.h"

#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkFont.h"
#include "third_party/skia/include/core/SkFontArguments.h"
#include "third_party/skia/include/core/SkFontMetrics.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkString.h"  // nogncheck
#include "third_party/skia/include/core/SkTypeface.h"

namespace {

constexpr auto kAngleSkew = std::to_array<const int8_t>({
    -0,  -2,  -3,  -5,  -7,  -9,  -11, -12, -14, -16, -18, -19, -21, -23, -25,
    -27, -29, -31, -32, -34, -36, -38, -40, -42, -45, -47, -49, -51, -53, -55,
});

int GetSkewFromAngle(int angle) {
  // |angle| is non-positive so |-angle| is used as the index. Need to make sure
  // |angle| != INT_MIN since -INT_MIN is undefined.
  if (angle > 0 || angle == std::numeric_limits<int>::min() ||
      static_cast<size_t>(-angle) >= std::size(kAngleSkew)) {
    return -58;
  }
  return kAngleSkew[-angle];
}

}  // namespace

// static
RetainPtr<CFX_Face> CFX_Face::New(RetainPtr<Retainable> cache_entry,
                                  RetainPtr<CFX_ReadOnlySpanStream> font_stream,
                                  uint32_t face_index) {
  CFX_FontMgr* font_mgr = CFX_GEModule::Get()->GetFontMgr();
  pdfium::span<const uint8_t> data = font_stream->span();

  sk_sp<SkTypeface> typeface = font_mgr->MakeSkTypeface(data);
  if (!typeface) {
    return nullptr;
  }
  return pdfium::WrapRetain(new CFX_Face(
      std::move(cache_entry), std::move(font_stream), std::move(typeface)));
}

CFX_Face::CFX_Face(RetainPtr<Retainable> cache_entry,
                   RetainPtr<CFX_ReadOnlySpanStream> font_stream,
                   sk_sp<SkTypeface> typeface)
    : cache_entry_(std::move(cache_entry)),
      font_stream_(std::move(font_stream)),
      typeface_(std::move(typeface)) {
  DCHECK(typeface_);
}

CFX_Face::~CFX_Face() = default;

bool CFX_Face::HasGlyphNames() const {
  return typeface_->getTableSize(SkSetFourByteTag('p', 'o', 's', 't')) > 0;
}

bool CFX_Face::IsTtOt() const {
  return true;
}

bool CFX_Face::IsTricky() const {
  return false;
}

bool CFX_Face::IsFixedWidth() const {
  return typeface_->isFixedPitch();
}

bool CFX_Face::IsItalic() const {
  return typeface_->fontStyle().slant() != SkFontStyle::kUpright_Slant;
}

bool CFX_Face::IsBold() const {
  return typeface_->fontStyle().weight() >= SkFontStyle::kBold_Weight;
}

ByteString CFX_Face::GetFamilyName() const {
  SkString familyName;
  typeface_->getFamilyName(&familyName);
  return ByteString(familyName.c_str());
}

ByteString CFX_Face::GetStyleName() const {
  return ByteString();
}

FX_RECT CFX_Face::GetBBox() const {
  SkRect bounds = typeface_->getBounds();
  return FX_RECT(bounds.left(), bounds.top(), bounds.right(), bounds.bottom());
}

uint16_t CFX_Face::GetUnitsPerEm() const {
  return typeface_->getUnitsPerEm();
}

int16_t CFX_Face::GetAscender() const {
  return 0;
}

int16_t CFX_Face::GetDescender() const {
  return 0;
}

pdfium::span<const uint8_t> CFX_Face::GetData() const {
  return font_stream_->span();
}

size_t CFX_Face::GetSfntTable(uint32_t table, pdfium::span<uint8_t> buffer) {
  if (buffer.empty()) {
    return typeface_->getTableSize(table);
  }
  return typeface_->getTableData(table, 0, buffer.size(), buffer.data());
}

int CFX_Face::GetGlyphCount() const {
  return typeface_->countGlyphs();
}

std::unique_ptr<CFX_GlyphBitmap> CFX_Face::RenderGlyph(
    uint32_t glyph_index,
    bool font_style,
    bool is_vertical,
    const CFX_Matrix& matrix,
    int dest_width,
    FontAntiAliasingMode anti_alias,
    const CFX_SubstFont* subst_font) {
  SkFont font(typeface_);
  font.setSize(64.0f);
  if (anti_alias == FontAntiAliasingMode::kMono) {
    font.setEdging(SkFont::Edging::kAlias);
  } else {
    font.setEdging(SkFont::Edging::kAntiAlias);
  }

  SkMatrix sk_matrix;
  sk_matrix.setAll(matrix.a / 64.0f, matrix.c / 64.0f, 0, matrix.b / 64.0f,
                   matrix.d / 64.0f, 0, 0, 0, 1);

  if (subst_font) {
    int angle = subst_font->italic_angle_;
    if (subst_font->subst_cjk_ && font_style) {
      angle = subst_font->italic_cjk_ ? -15 : 0;
    }
    if (angle) {
      int skew = GetSkewFromAngle(angle);
      if (is_vertical) {
        sk_matrix.preSkew(0, skew / 100.0f);
      } else {
        sk_matrix.preSkew(-skew / 100.0f, 0);
      }
    }
    if (subst_font->IsBuiltInGenericFont()) {
      AdjustVariationParams(glyph_index, dest_width, subst_font->weight_);
      font.setTypeface(typeface_);
    }
  }

  std::optional<SkPath> opt_path = font.getPath(glyph_index);
  if (!opt_path) {
    return nullptr;
  }
  SkPath path = *opt_path;
  path = path.makeTransform(sk_matrix);

  SkRect bounds = path.computeTightBounds();
  SkIRect ibounds = bounds.roundOut();
  if (ibounds.isEmpty()) {
    return nullptr;
  }

  SkBitmap sk_bitmap;
  sk_bitmap.allocPixels(SkImageInfo::MakeA8(ibounds.width(), ibounds.height()));
  sk_bitmap.eraseColor(SK_ColorTRANSPARENT);

  SkCanvas canvas(sk_bitmap);
  canvas.translate(-ibounds.left(), -ibounds.top());
  SkPaint paint;
  paint.setColor(SK_ColorBLACK);
  paint.setAntiAlias(anti_alias != FontAntiAliasingMode::kMono);
  canvas.drawPath(path, paint);

  RetainPtr<CFX_DIBitmap> new_bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  FXDIB_Format format = anti_alias == FontAntiAliasingMode::kMono
                            ? FXDIB_Format::k1bppMask
                            : FXDIB_Format::k8bppMask;

  if (!new_bitmap->Create(ibounds.width(), ibounds.height(), format)) {
    return nullptr;
  }

  const uint32_t dest_pitch = new_bitmap->GetPitch();
  for (int y = 0; y < ibounds.height(); ++y) {
    uint8_t* dest_row = new_bitmap->GetWritableScanline(y).data();
    const uint8_t* src_row =
        static_cast<const uint8_t*>(sk_bitmap.getAddr(0, y));
    if (format == FXDIB_Format::k8bppMask) {
      UNSAFE_BUFFERS(memcpy(dest_row, src_row, ibounds.width()));
    } else {
      UNSAFE_BUFFERS(memset(dest_row, 0, dest_pitch));
      for (int x = 0; x < ibounds.width(); ++x) {
        if (UNSAFE_BUFFERS(src_row[x]) > 127) {
          UNSAFE_BUFFERS(dest_row[x / 8] |= (1 << (7 - (x % 8))));
        }
      }
    }
  }

  return std::make_unique<CFX_GlyphBitmap>(ibounds.left(), ibounds.top(),
                                           new_bitmap);
}

std::unique_ptr<CFX_Path> CFX_Face::LoadGlyphPath(
    uint32_t glyph_index,
    int dest_width,
    bool is_vertical,
    const CFX_SubstFont* subst_font) {
  SkFont font(typeface_);
  font.setSize(64.0f);

  SkMatrix sk_matrix;
  sk_matrix.setAll(1.0f, 0, 0, 0, 1.0f, 0, 0, 0, 1);

  if (subst_font) {
    if (subst_font->italic_angle_) {
      int skew = GetSkewFromAngle(subst_font->italic_angle_);
      if (is_vertical) {
        sk_matrix.preSkew(0, skew / 100.0f);
      } else {
        sk_matrix.preSkew(-skew / 100.0f, 0);
      }
    }
    if (subst_font->IsBuiltInGenericFont()) {
      AdjustVariationParams(glyph_index, dest_width, subst_font->weight_);
      font.setTypeface(typeface_);
    }
  }

  std::optional<SkPath> opt_path = font.getPath(glyph_index);
  if (!opt_path) {
    return nullptr;
  }
  SkPath sk_path = *opt_path;
  sk_path = sk_path.makeTransform(sk_matrix);

  auto pPath = std::make_unique<CFX_Path>();

  SkPath::Iter iter(sk_path, false);
  SkPoint pts[4];
  SkPath::Verb verb;

  while ((verb = iter.next(pts)) != SkPath::kDone_Verb) {
    switch (verb) {
      case SkPath::kMove_Verb:
        pPath->AppendPoint(CFX_PointF(pts[0].x() / 64.0f, pts[0].y() / 64.0f),
                           CFX_Path::Point::Type::kMove);
        break;
      case SkPath::kLine_Verb:
        pPath->AppendPoint(CFX_PointF(pts[1].x() / 64.0f, pts[1].y() / 64.0f),
                           CFX_Path::Point::Type::kLine);
        break;
      case SkPath::kQuad_Verb: {
        float ctrl1_x = pts[0].x() + 2.0f / 3.0f * (pts[1].x() - pts[0].x());
        float ctrl1_y = pts[0].y() + 2.0f / 3.0f * (pts[1].y() - pts[0].y());
        float ctrl2_x = pts[2].x() + 2.0f / 3.0f * (pts[1].x() - pts[2].x());
        float ctrl2_y = pts[2].y() + 2.0f / 3.0f * (pts[1].y() - pts[2].y());
        pPath->AppendPoint(CFX_PointF(ctrl1_x / 64.0f, ctrl1_y / 64.0f),
                           CFX_Path::Point::Type::kBezier);
        pPath->AppendPoint(CFX_PointF(ctrl2_x / 64.0f, ctrl2_y / 64.0f),
                           CFX_Path::Point::Type::kBezier);
        pPath->AppendPoint(CFX_PointF(pts[2].x() / 64.0f, pts[2].y() / 64.0f),
                           CFX_Path::Point::Type::kBezier);
        break;
      }
      case SkPath::kConic_Verb: {
        float ctrl1_x = pts[0].x() + 2.0f / 3.0f * (pts[1].x() - pts[0].x());
        float ctrl1_y = pts[0].y() + 2.0f / 3.0f * (pts[1].y() - pts[0].y());
        float ctrl2_x = pts[2].x() + 2.0f / 3.0f * (pts[1].x() - pts[2].x());
        float ctrl2_y = pts[2].y() + 2.0f / 3.0f * (pts[1].y() - pts[2].y());
        pPath->AppendPoint(CFX_PointF(ctrl1_x / 64.0f, ctrl1_y / 64.0f),
                           CFX_Path::Point::Type::kBezier);
        pPath->AppendPoint(CFX_PointF(ctrl2_x / 64.0f, ctrl2_y / 64.0f),
                           CFX_Path::Point::Type::kBezier);
        pPath->AppendPoint(CFX_PointF(pts[2].x() / 64.0f, pts[2].y() / 64.0f),
                           CFX_Path::Point::Type::kBezier);
        break;
      }
      case SkPath::kCubic_Verb:
        pPath->AppendPoint(CFX_PointF(pts[1].x() / 64.0f, pts[1].y() / 64.0f),
                           CFX_Path::Point::Type::kBezier);
        pPath->AppendPoint(CFX_PointF(pts[2].x() / 64.0f, pts[2].y() / 64.0f),
                           CFX_Path::Point::Type::kBezier);
        pPath->AppendPoint(CFX_PointF(pts[3].x() / 64.0f, pts[3].y() / 64.0f),
                           CFX_Path::Point::Type::kBezier);
        break;
      case SkPath::kClose_Verb:
        pPath->ClosePath();
        break;
      case SkPath::kDone_Verb:
        break;
    }
  }

  if (pPath->GetPoints().empty()) {
    return nullptr;
  }

  pPath->ClosePath();
  return pPath;
}

int CFX_Face::GetGlyphTTWidth() const {
  return 0;
}

int CFX_Face::GetGlyphWidth(uint32_t glyph_index,
                            int dest_width,
                            int weight,
                            const CFX_SubstFont* subst_font) {
  if (subst_font && subst_font->IsBuiltInGenericFont()) {
    AdjustVariationParams(glyph_index, dest_width, weight);
  }

  SkFont font(typeface_);
  font.setSize(typeface_->getUnitsPerEm());
  font.setHinting(SkFontHinting::kNone);
  font.setLinearMetrics(true);

  SkScalar width;
  SkGlyphID glyph = glyph_index;
  font.getWidthsBounds(SkSpan<const SkGlyphID>(&glyph, 1),
                       SkSpan<SkScalar>(&width, 1), SkSpan<SkRect>(), nullptr);

  return static_cast<int>(width * 1000 / typeface_->getUnitsPerEm());
}

ByteString CFX_Face::GetGlyphName(uint32_t glyph_index) {
  return ByteString();
}

int CFX_Face::GetCharIndex(uint32_t code) {
  SkUnichar unichar = code;
  SkGlyphID glyph_id;
  typeface_->unicharsToGlyphs(SkSpan<const SkUnichar>(&unichar, 1),
                              SkSpan<SkGlyphID>(&glyph_id, 1));
  return glyph_id;
}

int CFX_Face::GetNameIndex(const char* name) {
  return 0;
}

FX_RECT CFX_Face::GetGlyphBBox() const {
  SkRect bounds = typeface_->getBounds();
  int em = GetUnitsPerEm();
  if (em == 0) {
    return FX_RECT();
  }
  return FX_RECT(bounds.left() * 1000 / em, bounds.top() * 1000 / em,
                 bounds.right() * 1000 / em, bounds.bottom() * 1000 / em);
}

std::optional<FX_RECT> CFX_Face::GetFontGlyphBBox(uint32_t glyph_index) {
  SkFont font(typeface_);
  font.setSize(typeface_->getUnitsPerEm());
  font.setHinting(SkFontHinting::kNone);
  font.setLinearMetrics(true);

  SkRect bounds;
  SkGlyphID glyph = glyph_index;
  font.getWidthsBounds(SkSpan<const SkGlyphID>(&glyph, 1), SkSpan<SkScalar>(),
                       SkSpan<SkRect>(&bounds, 1), nullptr);

  int em = GetUnitsPerEm();
  if (em == 0) {
    return std::nullopt;
  }

  return FX_RECT(bounds.left() * 1000 / em, bounds.top() * 1000 / em,
                 bounds.right() * 1000 / em, bounds.bottom() * 1000 / em);
}

FX_RECT CFX_Face::GetCharBBox(uint32_t code, int glyph_index) {
  return GetFontGlyphBBox(glyph_index).value_or(FX_RECT());
}

std::vector<CharCodeAndIndex> CFX_Face::GetCharCodesAndIndices(
    char32_t max_char) {
  std::vector<CharCodeAndIndex> results;
  for (char32_t c = 0; c <= max_char; ++c) {
    SkUnichar unichar = c;
    SkGlyphID glyph_id;
    typeface_->unicharsToGlyphs(SkSpan<const SkUnichar>(&unichar, 1),
                                SkSpan<SkGlyphID>(&glyph_id, 1));
    if (glyph_id != 0) {
      results.push_back({static_cast<uint32_t>(c), glyph_id});
    }
  }
  return results;
}

CFX_Face::CharMap CFX_Face::GetCurrentCharMap() const {
  return nullptr;
}

std::optional<fxge::FontEncoding> CFX_Face::GetCurrentCharMapEncoding() const {
  return std::nullopt;
}

CFX_Face::CharMapId CFX_Face::GetCharMapIdByIndex(size_t index) const {
  return {0, 0};
}

int CFX_Face::GetCharMapPlatformIdByIndex(size_t index) const {
  return 0;
}

int CFX_Face::GetCharMapEncodingIdByIndex(size_t index) const {
  return 0;
}

fxge::FontEncoding CFX_Face::GetCharMapEncodingByIndex(size_t index) const {
  return fxge::FontEncoding::kNone;
}

size_t CFX_Face::GetCharMapCount() const {
  return 0;
}

int CFX_Face::LoadGlyph(uint32_t glyph_index, bool scale) {
  return 0;
}

ByteString CFX_Face::GetPostscriptName() {
  SkString name;
  typeface_->getPostScriptName(&name);
  return ByteString(name.c_str());
}

CFX_Size CFX_Face::GetPixelSize() const {
  return CFX_Size(64, 64);
}

void CFX_Face::SetCharMap(CharMap map) {}
void CFX_Face::SetCharMapByIndex(size_t index) {}

bool CFX_Face::SelectCharMap(fxge::FontEncoding encoding) {
  return false;
}

#if defined(PDF_ENABLE_XFA) || BUILDFLAG(IS_ANDROID)
uint32_t CFX_Face::GetFontStyle() {
  uint32_t style = 0;
  if (IsBold()) {
    style |= pdfium::kFontStyleForceBold;
  }
  if (IsItalic()) {
    style |= pdfium::kFontStyleItalic;
  }
  if (IsFixedWidth()) {
    style |= pdfium::kFontStyleFixedPitch;
  }
  return style;
}

std::optional<std::array<uint32_t, 2>> CFX_Face::GetOs2CodePageRange() {
  return std::nullopt;
}

std::optional<std::array<uint8_t, 2>> CFX_Face::GetOs2Panose() {
  return std::nullopt;
}
#endif

#if defined(PDF_ENABLE_XFA)
bool CFX_Face::IsScalable() const {
  return true;
}
int CFX_Face::GetNumFaces() const {
  return 1;
}
std::optional<std::array<uint32_t, 4>> CFX_Face::GetOs2UnicodeRange() {
  return std::nullopt;
}
#endif

#if BUILDFLAG(IS_WIN)
bool CFX_Face::CanEmbed() {
  return true;
}
#endif

SkTypeface* CFX_Face::GetOrCreateSkTypeface() {
  return typeface_.get();
}

void CFX_Face::AdjustVariationParams(int glyph_index,
                                     int dest_width,
                                     int weight) {
  if (weight > 0 || dest_width > 0) {
    SkFontArguments::VariationPosition::Coordinate coords[2];
    int count = 0;
    if (weight > 0) {
      UNSAFE_BUFFERS(coords[count].axis = SkSetFourByteTag('w', 'g', 'h', 't'));
      UNSAFE_BUFFERS(coords[count].value = weight);
      count++;
    }
    if (dest_width > 0) {
      UNSAFE_BUFFERS(coords[count].axis = SkSetFourByteTag('w', 'd', 't', 'h'));
      UNSAFE_BUFFERS(coords[count].value = dest_width);
      count++;
    }
    SkFontArguments args;
    args.setVariationDesignPosition({coords, count});
    sk_sp<SkTypeface> new_typeface = typeface_->makeClone(args);
    if (new_typeface) {
      typeface_ = new_typeface;
    }
  }
}
