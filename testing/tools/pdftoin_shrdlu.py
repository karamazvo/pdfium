#!/usr/bin/env python3
# Copyright 2025 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Convert PDF files to .in format on a best-effort basis.
"""

import argparse
import re
import sys
from pathlib import Path


class PDFParser:

  def __init__(self, pdf_data):
    self.data = pdf_data
    self.objects = {}

  def parse(self):
    # Find all objects in the PDF
    obj_pattern = rb'(\d+)\s+(\d+)\s+obj\s*(.*?)\s*endobj'
    matches = re.finditer(obj_pattern, self.data, re.DOTALL)

    for match in matches:
      obj_num = int(match.group(1))
      gen_num = int(match.group(2))
      obj_content = match.group(3)

      self.objects[(obj_num, gen_num)] = obj_content

    return self.objects

  def format_object_content(self, content):
    """Format object content, handling dictionaries and streams."""
    content_str = content.decode('latin-1', errors='replace')

    # Check if this is a stream object
    if 'stream' in content_str and 'endstream' in content_str:
      # Split into dictionary and stream parts
      stream_start = content_str.find('stream')
      dict_part = content_str[:stream_start].strip()
      stream_part = content_str[stream_start:]

      # Replace /Length with {{streamlen}}
      dict_part = re.sub(r'/Length\s+\d+', '{{streamlen}}', dict_part)

      # Extract actual stream data
      stream_match = re.search(r'stream\s*(.*?)\s*endstream', stream_part,
                               re.DOTALL)
      if stream_match:
        stream_data = stream_match.group(1).strip()

        # Check if we need hex conversion
        hex_converted = False
        try:
          # Try to decode as text first
          stream_data.encode('ascii')
          formatted_stream = stream_data
        except UnicodeEncodeError:
          # If binary, convert to hex format
          formatted_stream = self.format_hex_data(stream_data.encode('latin-1'))
          hex_converted = True

        # Update Filter if hex conversion occurred
        if hex_converted:
          dict_part = self.prepend_ascii_hex_decode_filter(dict_part)

        return f"{dict_part}\nstream\n{formatted_stream}\nendstream"

    # Regular dictionary object
    content_str = re.sub(r'/Length\s+\d+', '{{streamlen}}', content_str)
    return content_str.strip()

  def prepend_ascii_hex_decode_filter(self, dict_str):
    """Prepend /ASCIIHexDecode to /Filter, creating array if needed."""
    # Check if /Filter exists
    filter_match = re.search(r'/Filter\s+(\S+(?:\s+\S+)*?)(?=\s*/|\s*>>)',
                             dict_str)

    if filter_match:
      existing_filter = filter_match.group(1).strip()

      # Check if it's already an array
      if existing_filter.startswith('['):
        # It's an array, prepend to it
        new_filter = existing_filter.replace('[', '[/ASCIIHexDecode ', 1)
      else:
        # Single filter, make it an array
        new_filter = f'[/ASCIIHexDecode {existing_filter}]'

      # Replace the filter
      dict_str = dict_str[:filter_match.start(1)] + new_filter + dict_str[
          filter_match.end(1):]
    else:
      # No /Filter exists, add it before the closing >>
      # Find a good place to insert (before >> or after last entry)
      insert_pos = dict_str.rfind('>>')
      if insert_pos == -1:
        insert_pos = len(dict_str)

      # Insert with proper spacing
      dict_str = dict_str[:insert_pos].rstrip(
      ) + '\n  /Filter /ASCIIHexDecode' + dict_str[insert_pos:]

    return dict_str

  def format_hex_data(self, data):
    """Format binary data as hex with proper spacing."""
    hex_str = data.hex()
    # Group into 4-character chunks
    chunks = [hex_str[i:i + 4] for i in range(0, len(hex_str), 4)]
    # Join with spaces, 8 chunks per line
    lines = []
    for i in range(0, len(chunks), 8):
      line = ' '.join(chunks[i:i + 8])
      lines.append(line)
    lines.append('>')
    return '\n'.join(lines)


def pdf_to_in(pdf_path, output_path=None):
  with open(pdf_path, 'rb') as f:
    pdf_data = f.read()

  # Parse PDF
  parser = PDFParser(pdf_data)
  objects = parser.parse()

  # Build output
  output_lines = ['{{header}}', '']

  # Sort objects by number
  sorted_objects = sorted(objects.keys(), key=lambda x: (x[0], x[1]))

  # Add objects
  for obj_num, gen_num in sorted_objects:
    obj_content = objects[(obj_num, gen_num)]
    formatted_content = parser.format_object_content(obj_content)

    output_lines.append(f'{{{{object {obj_num} {gen_num}}}}}')
    output_lines.append(formatted_content)
    output_lines.append('endobj')
    output_lines.append('')

  output_lines.append('{{xref}}')
  output_lines.append('{{trailer}}')
  output_lines.append('{{startxref}}')
  output_lines.append('%%EOF')
  output_text = '\n'.join(output_lines)

  if output_path:
    with open(output_path, 'w', encoding='utf-8') as f:
      f.write(output_text)
    print(f"Converted PDF written to: {output_path}")
  else:
    print(output_text)

  return output_text


def main():
  parser = argparse.ArgumentParser(
      description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument('input')
  parser.add_argument('-o', '--output')
  args = parser.parse_args()
  pdf_to_in(args.input, args.output)


if __name__ == '__main__':
  main()
