// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Based on https://github.com/googlefonts/fontations/pull/1820

fn main() {
    cxx_build::bridge("src/main.rs")
        .file("src/outlines.cpp")
        .compile("cxx-outlines");

    println!("cargo:rerun-if-changed=src/outlines.cpp");
    println!("cargo:rerun-if-changed=src/outlines.h");
}
