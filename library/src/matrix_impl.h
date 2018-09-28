/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**  matrix internals @file */

#ifndef sa_matrix_impl_h_
#define sa_matrix_impl_h_

#include "matrix.h"

struct sa_matrix_int {
  int rows;
  int cols;
  int v[];
};

struct sa_matrix_flt {
  int   rows;
  int   cols;
  float v[];
};

#endif
