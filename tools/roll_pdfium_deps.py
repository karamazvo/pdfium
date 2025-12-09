#!/usr/bin/env python3
# Copyright 2025 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import re
import sys


class DepsParser:

  def __init__(self, deps_content):
    self._deps_content = deps_content
    self.deps = {}
    self.vars = {}
    # self.Str and self.Var are used by exec() below.
    self.Str = lambda s: s
    self.Var = lambda v: self.vars.get(v, f"Var('{v}')")
    exec(self._deps_content, self.__dict__)


def find_path_for_revision_in_deps(deps_dict, revision):
  """Finds the deps path that corresponds to a given revision string."""
  for path, dep_info in deps_dict.items():
    if revision in str(dep_info):
      return path
  return None


def get_revision_from_dep_value(dep_value):
  url = dep_value
  if isinstance(dep_value, dict):
    # This is for CIPD packages.
    if 'packages' in dep_value:
      for package in dep_value['packages']:
        if 'version' in package:
          return package['version']
    url = dep_value.get('url')

  if not url or not isinstance(url, str):
    return None

  match = re.search(r'@(.+)', url)
  if match:
    return match.group(1)
  return None


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('chromium_deps_file', help='Path to Chromium DEPS file')
  parser.add_argument('pdfium_deps_file', help='Path to the PDFium DEPS file')
  parser.add_argument('deps_entry', help='DEPS entry to roll')
  args = parser.parse_args()

  UNSUPPORTED_ENTRIES = ['pdfium_tests_revision', 'test_fonts_revision']
  if args.deps_entry in UNSUPPORTED_ENTRIES:
    print(f'Rolling {args.deps_entry} is not supported.', file=sys.stderr)
    return 1

  if os.path.abspath(args.chromium_deps_file) == os.path.abspath(
      args.pdfium_deps_file):
    print('DEPS files must be different.', file=sys.stderr)
    return 1

  with open(args.chromium_deps_file, 'r') as f:
    chromium_deps_content = f.read()
  with open(args.pdfium_deps_file, 'r') as f:
    pdfium_deps_content = f.read()

  chromium_deps_parser = DepsParser(chromium_deps_content)
  pdfium_deps_parser = DepsParser(pdfium_deps_content)

  # Map PDFium DEPS variable names to Chromium DEPS variable names.
  VAR_MAPPING = {'gtest_revision': 'googletest_revision'}

  # Map PDFium DEPS paths to Chromium DEPS paths.
  # Most mapping are just path/to/foo to src/path/to/foo, but there are some
  # exceptions.
  DEPS_PATH_MAPPING = {
      'third_party/android_toolchain/ndk': 'src/third_party/android_toolchain',
  }

  # In PDFium DEPS, only search vars for the entry.
  if args.deps_entry not in pdfium_deps_parser.vars:
    print(
        f'Entry "{args.deps_entry}" not found in PDFium DEPS.', file=sys.stderr)
    return 1

  pdfium_revision = pdfium_deps_parser.vars.get(args.deps_entry)
  deps_path = find_path_for_revision_in_deps(pdfium_deps_parser.deps,
                                             pdfium_revision)
  if not deps_path:
    print(
        f'Could not find path for var "{args.deps_entry}" in PDFium DEPS file.',
        file=sys.stderr)
    return 1

  # Check if the dependency is a CIPD package, which is not supported.
  pdfium_dep_value = pdfium_deps_parser.deps.get(deps_path)
  is_cipd = (
      isinstance(pdfium_dep_value, dict) and
      pdfium_dep_value.get('dep_type') == 'cipd')

  # In Chromium DEPS, search vars and deps.
  chromium_revision = None
  chromium_deps_entry_key = VAR_MAPPING.get(args.deps_entry, args.deps_entry)
  chromium_revision = chromium_deps_parser.vars.get(chromium_deps_entry_key)

  if not chromium_revision:
    # Not in vars, try deps.
    if deps_path:
      chromium_path = DEPS_PATH_MAPPING.get(deps_path, 'src/' + deps_path)
      chromium_dep_value = chromium_deps_parser.deps.get(chromium_path)
      if chromium_dep_value:
        chromium_revision = get_revision_from_dep_value(chromium_dep_value)

  if not chromium_revision:
    if is_cipd:
      print(
          f'CIPD entry "{args.deps_entry}" not found in Chromium DEPS.',
          file=sys.stderr)
      return 1

    # If chromium_revision is still None, it means it wasn't found in Chromium
    # DEPS. In which case, suggest rolling to ToT.
    print(f'roll-dep {deps_path} --ignore-dirty-tree --no-log')
    return 0

  if chromium_revision == pdfium_revision:
    print('Revisions are the same.')
    return 0

  if is_cipd:
    print(f"blah {chromium_revision} vs. {pdfium_revision}.")
  else:
    print(f'roll-dep {deps_path} --roll-to {chromium_revision} '
          '--ignore-dirty-tree --no-log')
  return 0


if __name__ == '__main__':
  main()
