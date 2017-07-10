/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**  Piece_wise parabolic predicition (P2).
 *   Dynamic calculation of quantiles and histograms without storing
 *   observations. http://www.cs.wustl.edu/~jain/papers/ftp/psqr.pdf
 *  @file */

#ifndef sa_p2_impl_h_
#define sa_p2_impl_h_

#include "p2.h"

#define QUANTILE_MARKERS 5

struct sa_p2_quantile {
  unsigned short cnt;
  float  p;
  double q[QUANTILE_MARKERS];
  double n[QUANTILE_MARKERS];
  double n1[QUANTILE_MARKERS];
};


struct sa_p2_histogram {
  unsigned short cnt;
  unsigned short b;
  double data[];
};

#endif
