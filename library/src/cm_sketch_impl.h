/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**  Count-min sketch @file */

#ifndef sa_cm_sketch_impl_h_
#define sa_cm_sketch_impl_h_

#include "cm_sketch.h"

extern const double g_eulers_number;

struct sa_cm_sketch {
  uint64_t item_count;
  uint64_t unique_count;
  uint32_t depth;
  uint32_t width;
  uint32_t counts[];
};

#endif
