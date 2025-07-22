#!/usr/bin/env python3
# Copyright 2025 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import re
import tokenize
import io


def convert_variable_name(name):
  """
    Converts a Hungarian-style variable name to Google-style snake_case.

    Examples:
    - pFakeItem -> fake_item
    - pIPCHandler -> ipc_handler
    - pEFile -> e_file
    - pAVeryLongButLegalIdentifierToReFormatIntoFoo -> a_very_long_but_legal_identifier_to_re_format_into_foo
    """
  # Step 1: Remove leading 'p' if present and followed by an uppercase letter
  # This specifically targets the 'p' prefix for pointers in Hungarian notation.
  if len(name) > 1 and name[0] == 'p' and name[1].isupper():
    name = name[1:]

  # Step 2: Convert CamelCase to snake_case, handling consecutive capital letters
  # This regular expression handles two main cases for inserting underscores:
  # 1. `([A-Z][a-z])`: Matches an uppercase letter followed by a lowercase letter (e.g., 'Item', 'Handler').
  #    An underscore is inserted before such a sequence if it's not at the very beginning of the string.
  #    Example: 'FakeItem' becomes 'Fake_Item'.
  # 2. `([A-Z]{2,}(?=[A-Z][a-z]|\d|\Z))`: Matches two or more consecutive uppercase letters
  #    (e.g., 'IPC', 'HTTP') that are followed by:
  #    - An uppercase letter then a lowercase letter (e.g., 'IPCHandler' where 'IPC' is followed by 'H')
  #    - A digit (e.g., 'VAR1')
  #    - The end of the string (`\Z`)
  #    This ensures that multi-capital abbreviations like 'IPC' or 'HTTP' are treated as a single unit
  #    and an underscore is inserted *after* them if a new word follows.
  # The `(?<!^)` is a negative lookbehind assertion, ensuring no underscore is inserted at the very beginning.
  s = re.sub(r'(?<!^)([A-Z][a-z]|[A-Z]{2,}(?=[A-Z][a-z]|\d|\Z))', r'_\1', name)

  # Ensure the entire string is lowercase.
  # If the string starts with an uppercase letter after 'p' removal (e.g., 'FakeItem' -> 'Fake_Item'),
  # this step makes the first letter lowercase.
  # This covers cases like 'EFile' becoming 'e_file'.
  s = s[0].lower() + s[1:] if s and s[0].isupper() else s

  # Finally, convert the whole string to lowercase and replace any double underscores
  # that might have been introduced by edge cases of the regex (though unlikely with current pattern)
  return s.lower().replace('__', '_')


def refactor_file_content(content):
  """
    Refactors the content of a Python file, converting Hungarian-style variable names.

    Args:
        content (str): The entire content of the Python file.

    Returns:
        str: The modified content with refactored variable names.
    """
  # Split content into lines, keeping the original line endings
  lines = content.splitlines(keepends=True)

  # Use io.StringIO to simulate a file for the tokenize module, which expects a readline method.
  source_lines_iterator = iter(lines)

  def readline_func():
    try:
      return next(source_lines_iterator)
    except StopIteration:
      return ""

  tokens = list(tokenize.generate_tokens(readline_func))

  # Store modifications as (start_row, start_col, end_row, end_col, new_value)
  # We store these and apply them in reverse order to avoid issues with index shifts
  # when modifying the list of lines.
  modifications = []

  for toktype, tokval, (srow, scol), (erow, ecol), line_text in tokens:
    # We are only interested in NAME tokens, which represent identifiers (variables, functions, classes, etc.)
    if toktype == tokenize.NAME:
      # Check if the token value matches the Hungarian style pattern:
      # starts with 'p', followed by an uppercase letter, then any alphanumeric characters.
      if re.fullmatch(r'p[A-Z][a-zA-Z0-9_]*', tokval):
        new_name = convert_variable_name(tokval)
        # Only record a modification if the name actually changes
        if new_name != tokval:
          modifications.append({
              'start_row': srow - 1,  # Convert to 0-based index for list
              'start_col': scol,
              'end_row': erow - 1,  # Convert to 0-based index for list
              'end_col': ecol,
              'new_val': new_name
          })

  # Apply modifications from the end of the file/line to the beginning.
  # This prevents index errors as strings are modified.
  modifications.sort(
      key=lambda x: (x['start_row'], x['start_col']), reverse=True)

  for mod in modifications:
    srow, scol = mod['start_row'], mod['start_col']
    erow, ecol = mod['end_row'], mod['end_col']
    new_val = mod['new_val']

    # Python's tokenize module guarantees that NAME tokens are always on a single line.
    # So, we only need to modify the specific line.
    line_to_modify = lines[srow]
    lines[srow] = line_to_modify[:scol] + new_val + line_to_modify[ecol:]

  return "".join(lines)


def main():
  """
    Main function to parse arguments and process files.
    """
  parser = argparse.ArgumentParser(
      description="Convert Hungarian-style variable names to Google-style snake_case in Python files."
  )
  parser.add_argument(
      'filenames',
      metavar='FILE',
      nargs='+',  # Accepts one or more filenames
      help='One or more Python filenames to process in-place.')
  args = parser.parse_args()

  for filename in args.filenames:
    # Check if the file exists
    if not os.path.exists(filename):
      print(f"Error: File '{filename}' not found. Skipping.")
      continue

    try:
      # Read the original content of the file
      with open(filename, 'r', encoding='utf-8') as f:
        original_content = f.read()

      # Refactor the content
      modified_content = refactor_file_content(original_content)

      # If changes were made, write the modified content back to the file
      if modified_content != original_content:
        with open(filename, 'w', encoding='utf-8') as f:
          f.write(modified_content)
        print(f"Successfully refactored '{filename}'.")
      else:
        print(
            f"No Hungarian-style variables found or no changes needed in '{filename}'."
        )

    except Exception as e:
      # Catch any other exceptions during file processing
      print(f"Error processing '{filename}': {e}")


if __name__ == '__main__':
  main()
