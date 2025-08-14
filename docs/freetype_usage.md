# FreeType Usage in PDFium

This document was created with the help of gemini-cli. It summarizes the usage of the FreeType object across the PDFium codebase, in particular based on where `FT_Face` and the `CFX_Face` wrapper are used. The `FT_Face` object is a handle to a specific font face, which corresponds to a single font style in a font file.

## `core/fxge/cfx_face.h`

This header file defines the `CFX_Face` class, which is a wrapper around FreeType's `FT_Face`.

- **`FXFT_FaceRec* GetRec()`**: This method returns a raw pointer to the underlying `FT_FaceRec` struct, which is typedef-ed to `FT_Face`. This provides direct access to the FreeType face object for other parts of the system.
- **`ScopedFXFTFaceRec`**: The class holds the `FT_Face` in a `std::unique_ptr` with a custom deleter (`FXFTFaceRecDeleter`), ensuring that `FT_Done_Face` is called automatically for proper resource management.

## `core/fxge/cfx_face.cpp`

This is the implementation file for `CFX_Face` and contains the bulk of the direct interaction with the `FT_Face` object.

- **`CFX_Face::New()`**: Uses `FT_New_Memory_Face` to create a new `FT_Face` object from font data in memory. This is the primary way font faces are created from embedded font data.
- **`CFX_Face::Open()`**: Uses `FT_Open_Face` to create a new `FT_Face` from a font file, using `FT_Open_Args` to specify how to open the font data.
- **Glyph Loading and Rendering**:
    - `FT_Load_Glyph`: Loads a single glyph into the face's glyph slot. It's used with flags like `FT_LOAD_NO_SCALE`, `FT_LOAD_NO_BITMAP`, and `FT_LOAD_PEDANTIC` to control the loading process.
    - `FT_Render_Glyph`: Renders the currently loaded glyph into a bitmap. This is used to generate bitmap representations of glyphs for display.
    - `FT_Get_Glyph`: Used to extract a standalone glyph object from the face's glyph slot after loading.
    - `FT_Outline_Decompose`: Decomposes a glyph's outline into a series of points and curves, which is used to create `CFX_Path` objects for vector rendering.
- **Character and Glyph Indexing**:
    - `FT_Get_Char_Index`: Translates a character code into a glyph index for the currently selected character map (`cmap`).
    - `FT_Get_Name_Index`: Retrieves the glyph index for a given glyph name.
    - `FT_Get_Glyph_Name`: Retrieves the name of a glyph from its index.
- **Font Metrics and Properties**:
    - Accesses members of the `FT_FaceRec` struct directly (e.g., `face->units_per_EM`, `face->ascender`, `face->descender`, `face->bbox`) to get font-wide metrics.
    - `FT_Get_Sfnt_Table`: Accesses raw SFNT tables (like 'OS/2') from TrueType/OpenType fonts to get specific font properties.
    - `FT_Load_Sfnt_Table`: Loads an SFNT table into a buffer.
- **Character Map (`cmap`) Management**:
    - `FT_Select_Charmap`: Selects a character map based on its encoding (e.g., `FT_ENCODING_UNICODE`).
    - `FT_Set_Charmap`: Sets the active character map for the face.
- **Transformations**:
    - `FT_Set_Pixel_Sizes`: Sets the nominal pixel size for the font, which affects how glyphs are hinted and rendered.
- **Multiple Master and Variation Fonts**:
    - `FT_Get_MM_Var`: Retrieves the Multiple Master variations descriptor.
    - `FT_Set_MM_Design_Coordinates`: Sets the active coordinates for a variation font, allowing for for selection of a specific instance (e.g., weight, width).

## `core/fxge/freetype/fx_freetype.h`

This header defines type aliases and deleters for FreeType objects to ensure proper RAII (Resource Acquisition Is Initialization).

- **`FXFT_FaceRec`**: An alias for `struct FT_FaceRec_`.
- **`FXFTFaceRecDeleter`**: A custom deleter for `std::unique_ptr` that calls `FT_Done_Face` to release the `FT_Face` object. This is crucial for preventing resource leaks.
- **`ScopedFXFTFaceRec`**: A `std::unique_ptr` using `FXFTFaceRecDeleter`, providing safe, automatic memory management for `FT_Face` objects.

## `core/fxge/freetype/fx_freetype.cpp`

This file contains utility functions related to FreeType.

- **`ScopedFXFTMMVar`**: A helper class to manage `FT_MM_Var` objects, which are used for Multiple Master and variation fonts. It calls `FT_Get_MM_Var` to get the variation descriptor from an `FT_Face`.

## `core/fpdfapi/font/cpdf_simplefont.cpp`

This file deals with simple fonts in a PDF and uses `CFX_Face` (and thus `FT_Face`) to get font metrics.

- **`LoadCharMetrics()`**: This function indirectly uses `FT_Face` through the `CFX_Face` wrapper.
    - It calls `FT_Load_Glyph` with `FT_LOAD_NO_SCALE` to load unscaled glyph metrics. This allows the application to get the glyph's "ideal" outline shape and metrics in font units.
    - It then retrieves the horizontal advance and bounding box for the loaded glyph.

## `core/fpdfapi/font/cpdf_type1font.cpp`

This file handles Type 1 fonts. The primary interaction with FreeType is in `CPDF_Type1Font::LoadGlyphMap()` to map character codes to glyph indices.

- **Character Map Selection**: It uses `face->SelectCharMap()` and other `CFX_Face` helpers to find and select the most appropriate character map in the font. This is important for handling different encodings and symbolic fonts correctly.
- **Glyph Index Lookup**: It uses `face->GetCharIndex()` to map character codes to glyph indices and `face->GetNameIndex()` to map Adobe glyph names to indices. This is a key step in resolving the characters in the PDF to the actual glyphs in the font.
- **Glyph Name Retrieval**: `face->GetGlyphName()` is used to retrieve the name of a glyph, which can then be used to look up its Unicode value from the Adobe Glyph List.

## `core/fxge/cfx_font.h`

This header defines a higher-level font abstraction, `CFX_Font`.

- It holds a `RetainPtr<CFX_Face>`, which in turn manages the `FT_Face`.
- **`GetFaceRec()`**: Provides access to the raw `FT_Face` pointer, similar to `CFX_Face::GetRec()`.

## `core/fxge/scoped_font_transform.cpp`

This file defines a helper class for applying temporary transformations to an `FT_Face`.

- **`ScopedFontTransform`**: The constructor takes a `CFX_Face` and an `FT_Matrix` and calls `FT_Set_Transform`. This function sets the transformation matrix that will be applied to glyphs when they are loaded.
- The destructor resets the transformation on the `FT_Face` to the identity matrix, ensuring that the transformation is only applied within the scope of the `ScopedFontTransform` object.

## `CFX_Face` to `FT_Face` Usage Patterns

The `CFX_Face` class serves as a wrapper around the FreeType `FT_Face` object. The primary way to get the underlying `FT_Face` is through the `GetRec()` method. This pattern is used throughout the codebase to interact with the FreeType library.

- **`core/fpdfapi/font/cpdf_simplefont.cpp`**: In `LoadCharMetrics()`, `font_.GetFace()->GetRec()` is called to get the `FT_Face` for `FT_Load_Glyph`.
- **`core/fpdfapi/font/cpdf_cidfont.cpp`**: `UseCIDCharmap()` calls `face->SelectCharMap()` which in turn calls `FT_Select_Charmap` on the result of `GetRec()`.
- **`core/fxge/cfx_font.cpp`**: In `GetGlyphBBox()`, `face_->GetRec()` is used to get the `FT_Face` for calls to `FT_Set_Char_Size`, `FT_Load_Glyph`, and `FT_Get_Glyph`.
- **`core/fxge/cfx_unicodeencoding.cpp` and `core/fxge/cfx_unicodeencodingex.cpp`**: These files use `font_->GetFace()->GetCharIndex()` and `font_->GetFace()->SelectCharMap()`, which internally get the `FT_Face` via `GetRec()` to call `FT_Get_Char_Index` and `FT_Select_Charmap`.
- **`core/fxge/scoped_font_transform.cpp`**: The `ScopedFontTransform` constructor and destructor call `FT_Set_Transform` on the `FT_Face` obtained from `face_->GetRec()`.
- **`xfa/fgas/font/cfgas_fontmgr.cpp`**: On non-Windows platforms, this file enumerates system fonts. In the `RegisterFace` function, it creates a `CFX_Face` for each font and extracts detailed properties by calling a variety of `CFX_Face` methods. It reads style flags (`IsBold`, `IsItalic`, `IsFixedWidth`), OpenType 'OS/2' table information for character set support and classification (`GetOs2UnicodeRange`, `GetOs2CodePageRange`, `GetOs2Panose`), and the 'name' table for family names (`GetSfntTable`). It also directly accesses the `family_name` and `face_index` from the underlying `FT_FaceRec` struct.
- **`core/fxge/win32/cfx_psrenderer.cpp`**: `GenerateType42FontData()` uses `font->GetFace()` to get the `CFX_Face` and then accesses properties on the underlying `FT_Face` to generate Type 42 font data for PostScript output.

## `CFX_Face` Creation and Font Formats

This section details where `CFX_Face` objects are created and the format of the source font data.

### `CFX_Face::New()` (In-Memory Fonts)

This function is used when the font data is already loaded into a memory buffer.

- **`core/fxge/cfx_fontmgr.cpp` in `CFX_FontMgr::NewFixedFace()`**:
    - **Source**: The data can be from several places:
        1.  **Built-in Fonts**: For PDF's 14 standard fonts, the data is from pre-compiled byte arrays (e.g., `kFoxitSansFontData`). These are **Type 1 fonts**.
        2.  **Chrome Generic Fonts**: For generic "sans" and "serif" fonts, the data is from pre-compiled byte arrays (`kFoxitSansMMFontData`, `kFoxitSerifMMFontData`). These are **TrueType fonts**.
        3.  **System Font Cache**: Font data can be from a cache of system fonts, which are typically **TrueType/OpenType (SFNT)**.
- **`core/fxge/cfx_font.cpp` in `CFX_Font::LoadEmbedded()`**:
    - **Source**: The data comes from an embedded font stream within a PDF file. This call happens within `CPDF_Font::LoadFontDescriptor()` (see `core/fpdfapi/font/cpdf_font.cpp`, line 153), which is the common pathway for all embedded fonts defined in a PDF.
    - **Format**: The format of the font data is determined by the PDF's font descriptor dictionary, as specified in the PDF 1.7 specification (ISO 32000-1:2008), section 9.8.1. `LoadFontDescriptor()` looks for the font stream in these keys:
        - `/FontFile`: A stream containing a **Type 1** font program.
        - `/FontFile2`: A stream containing a **TrueType** font program.
        - `/FontFile3`: A stream containing a font program whose format is specified by the stream's `/Subtype` entry. This includes `Type1C` (Compact Font Format, or **CFF**), `CIDFontType0C` (CID-keyed **CFF**), and `OpenType`. An OpenType stream can contain either TrueType or CFF outlines.
    Therefore, the stream passed to `LoadEmbedded()` can contain a variety of font formats, and FreeType is responsible for identifying the correct format from the raw bytes. This is the most flexible entry point and handles the full range of font types that can be embedded in a PDF.
    #### High-Level Operations on Embedded Fonts
    Once an embedded font is loaded and a `CFX_Face` is created, it is used to perform several high-level operations necessary for PDF rendering and text processing:
    - **Character Code to Glyph Index Mapping**: This is the most fundamental operation. PDF text objects contain character codes, not glyph indices. The `CPDF_Font` subclasses (like `CPDF_Type1Font` and `CPDF_CIDFont`) use the `CFX_Face` to access the font's character maps (`cmaps`). They call methods like `face->GetCharIndex()` and `face->GetNameIndex()` to translate the PDF character codes into the specific glyph indices that FreeType needs to render the character.
    - **Retrieving Glyph Metrics**: For text layout and positioning, PDFium needs to know the width of each character. The `CPDF_SimpleFont::LoadCharMetrics()` function is called when width information isn't explicitly defined in the PDF. This function uses the `CFX_Face` to load a glyph (`FT_Load_Glyph`) and then retrieves its advance width from the `FT_FaceRec`'s glyph slot. This information is crucial for correctly positioning subsequent characters on a line.
    - **Retrieving Glyph Bounding Boxes**: To calculate the "ink bounds" of text for rendering, clipping, and hit-testing, PDFium needs the bounding box of each glyph. Similar to retrieving widths, the `CPDF_SimpleFont::LoadCharMetrics()` and `CPDF_CIDFont::GetCharBBox()` methods use the `CFX_Face` to load a glyph and then call `face->GetGlyphBBox()` to get its precise bounding box in font units.
    - **Extracting Glyph Outlines**: For high-quality display and printing, especially when zoomed in, PDFium renders text using its vector outline rather than a bitmap. The `CFX_GlyphCache` calls `CFX_Face::LoadGlyphPath()`, which uses FreeType's `FT_Outline_Decompose` function to convert the glyph's outline into a `CFX_Path` object. This path is then stroked or filled like any other vector graphic.
    - **Supporting Text Extraction**: To enable features like copy-pasting text from a PDF, PDFium needs to map character codes back to Unicode. While this primarily relies on the PDF's `/ToUnicode` map, the `FT_Face` is sometimes used to supplement this. For example, in `CPDF_Type1Font`, `face->GetGlyphName()` is used to get the Adobe glyph name for a character, which can then be mapped to a Unicode value using standard Adobe mapping tables.


### `CFX_Face::Open()` (File/Stream-Based Fonts)

This function is used when the font is to be loaded from a file path or a custom stream.

- **`core/fxge/android/cfpf_skiafontmgr.cpp` in `GetFontFace()`**:
    - **Source**: The `FT_Open_Args` are populated with a `pathname` to a font file on the Android file system (e.g., in `/system/fonts`).
    - **Format**: These are standard system fonts, which are **TrueType/OpenType (SFNT)**, with file extensions like `.ttf`, `.ttc`, or `.otf`.
- **`xfa/fgas/font/cfgas_fontmgr.cpp` in `LoadFace()`** (non-Windows builds):
    - **Source**: The `FT_Open_Args` are populated with a custom `FT_Stream` that reads from a system font file via an `IFX_SeekableReadStream`.
    - **Format**: This is used for XFA's font management and loads system fonts, which are **TrueType/OpenType (SFNT)**.
- **`core/fxge/cfx_font.cpp` in `CFX_Font::LoadFile()`** (for XFA builds):
    - **Source**: Similar to the above, this uses a custom `FT_Stream` that reads from an `IFX_SeekableReadStream`.
    - **Format**: This is also for XFA and loads **TrueType/OpenType (SFNT)** fonts.

In all observed cases, `CFX_Face::Open()` is used for SFNT-based container fonts (TrueType/OpenType), while `CFX_Face::New()` is used for both SFNT and non-SFNT fonts (like Type 1) that are already in memory.
