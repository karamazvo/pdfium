// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_FONT_LIBRARY_H_
#define CORE_FXGE_FONT_LIBRARY_H_

// An opaque class that wraps the handle to font libraries.
class FontLibrary {
 public:
  enum class Type {
    kFreeType,
  };

  virtual ~FontLibrary() = default;

  // Returns the font library type.
  virtual Type GetType() = 0;

  // Returns a handle to the actual font library. The classes that implement
  // this should also provide a utility function to cast it.
  virtual void* GetHandle() = 0;

  // Whether the font library supports hinting or not.
  virtual bool SupportsHinting() = 0;
};

#endif  // CORE_FXGE_FONT_LIBRARY_H_
