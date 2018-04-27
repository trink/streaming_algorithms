/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**  time_series internals @file */

#ifndef sa_time_series_impl_h_
#define sa_time_series_impl_h_

#include "time_series.h"

struct sa_time_series_int {
  uint64_t current_time;
  uint64_t ns_per_row;
  int rows;
  int v[];
};

#endif
