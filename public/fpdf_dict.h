// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PUBLIC_FPDF_DICT_H_
#define PUBLIC_FPDF_DICT_H_

#include "public/fpdfview.h"

typedef const struct fpdf_dictionary_t__* FPDF_DICTIONARY;

typedef struct {
  FPDF_BOOL success;
  int value;
} FPDF_RESULT_INT;

typedef struct {
  FPDF_BOOL success;
  FPDF_BOOL value;
} FPDF_RESULT_BOOL;

typedef struct {
  FPDF_BOOL success;
  float value;
} FPDF_RESULT_FLOAT;

#ifdef __cplusplus
extern "C" {
#endif

// Experimental API.
// Function: FPDF_GetPageDictionary
//       Gets the page dictionary as an FPDF_DICTIONARY, which can be passed
//       to helper functions to retrieve values associated with specific keys.
// Parameters:
//       doc        - Handle to the document.
//       page_index - Zero-based index of the page.
// Return value:
//       Returns an FPDF_DICTIONARY handle, or nullptr on failure.
FPDF_EXPORT FPDF_DICTIONARY FPDF_CALLCONV
FPDF_GetPageDictionary(FPDF_DOCUMENT doc, int page_index);

// Experimental API.
// Function: FPDF_DictionaryGetString
//       Gets a string value associated with a key from a dictionary.
// Parameters:
//       dictionary - Handle to the dictionary.
//       key        - The key to look up (as a C-style string).
//       buffer     - Buffer for holding the string value, encoded in UTF-16LE.
//       buflen     - The length of the buffer in bytes.
// Return value:
//       The number of bytes of the string value, including the terminator.
//       Returns 0 if the key does not exist or the value is not a string.
FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDF_DictionaryGetString(FPDF_DICTIONARY dictionary,
                         FPDF_BYTESTRING key,
                         void* buffer,
                         unsigned long buflen);

// Experimental API.
// Function: FPDF_DictionaryGetInt
//       Gets an integer value associated with a key from a dictionary.
// Parameters:
//       dictionary - Handle to the dictionary.
//       key        - The key to look up (as a C-style string).
// Return value:
//       Returns an FPDF_RESULT_INT. If the key exists and is a number,
//       'success' is TRUE and 'value' contains the integer.
FPDF_EXPORT FPDF_RESULT_INT FPDF_CALLCONV
FPDF_DictionaryGetInt(FPDF_DICTIONARY dictionary, FPDF_BYTESTRING key);

// Experimental API.
// Function: FPDF_DictionaryGetBool
//       Gets a boolean value associated with a key from a dictionary.
// Parameters:
//       dictionary - Handle to the dictionary.
//       key        - The key to look up (as a C-style string).
// Return value:
//       Returns an FPDF_RESULT_BOOL. If the key exists and is a boolean,
//       'success' is TRUE and 'value' contains the boolean.
FPDF_EXPORT FPDF_RESULT_BOOL FPDF_CALLCONV
FPDF_DictionaryGetBool(FPDF_DICTIONARY dictionary, FPDF_BYTESTRING key);

// Experimental API.
// Function: FPDF_DictionaryGetFloat
//       Gets a floating point value associated with a key from a dictionary.
// Parameters:
//       dictionary - Handle to the dictionary.
//       key        - The key to look up (as a C-style string).
// Return value:
//       Returns an FPDF_RESULT_FLOAT. If the key exists and is a number,
//       'success' is TRUE and 'value' contains the float.
FPDF_EXPORT FPDF_RESULT_FLOAT FPDF_CALLCONV
FPDF_DictionaryGetFloat(FPDF_DICTIONARY dictionary, FPDF_BYTESTRING key);

// Experimental API.
// Function: FPDF_DictionaryGetDict
//       Gets a nested dictionary associated with a key.
// Parameters:
//       dictionary - Handle to the dictionary.
//       key        - The key to look up.
// Return value:
//       Returns a handle to the dictionary, or nullptr on failure.
FPDF_EXPORT FPDF_DICTIONARY FPDF_CALLCONV
FPDF_DictionaryGetDict(FPDF_DICTIONARY dictionary, FPDF_BYTESTRING key);

// Experimental API.
// Function: FPDF_DictionaryGetRect
//       Gets a rectangle associated with a key.
// Parameters:
//       dictionary - Handle to the dictionary.
//       key        - The key to look up.
//       rect       - Pointer to receive the rectangle coordinates.
// Return value:
//       Returns true on success, false if dictionary is invalid or key not
//       found.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDF_DictionaryGetRect(FPDF_DICTIONARY dictionary,
                       FPDF_BYTESTRING key,
                       FS_RECTF* rect);

// Experimental API.
// Function: FPDF_DictionaryGetMatrix
//       Gets a transformation matrix associated with a key.
// Parameters:
//       dictionary - Handle to the dictionary.
//       key        - The key to look up.
//       matrix     - Pointer to receive the matrix values.
// Return value:
//       Returns true on success, false if dictionary is invalid or key not
//       found.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDF_DictionaryGetMatrix(FPDF_DICTIONARY dictionary,
                         FPDF_BYTESTRING key,
                         FS_MATRIX* matrix);

// TODO(crbug.com/42270701): Add array and stream support.

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // PUBLIC_FPDF_DICT_H_
