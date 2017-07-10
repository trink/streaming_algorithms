/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Running stats implementation @file */

#include "common.h"
#include "running_stats.h"

#include <math.h>
#include <stdlib.h>

void sa_init_running_stats(sa_running_stats *s)
{
  s->count = 0.0;
  s->mean = 0.0;
  s->sum = 0.0;
}


void sa_add_running_stats(sa_running_stats *s, double d)
{
  if (!isfinite(d)) return;

  double old_mean = s->mean;
  double old_sum = s->sum;

  if (++s->count == 1) {
    s->mean = d;
  } else {
    s->mean = old_mean + (d - old_mean) / s->count;
    s->sum = old_sum + (d - old_mean) * (d - s->mean);
  }
}


double sa_variance_running_stats(sa_running_stats *s)
{
  if (s->count < 2) return 0.0;
  return s->sum / (s->count - 1);
}


double sa_sd_running_stats(sa_running_stats *s)
{
  if (s->count < 2) return 0.0;
  return sqrt(s->sum / (s->count - 1));
}


char* sa_serialize_running_stats(sa_running_stats *s, size_t *len)
{
  *len = sizeof(double) * 3;
  char *buf = malloc(*len);
  if (!buf) {
    *len = 0;
    return NULL;
  }
  n2b(&s->count, buf, sizeof(double));
  n2b(&s->mean, buf + sizeof(double), sizeof(double));
  n2b(&s->sum, buf + sizeof(double) * 2, sizeof(double));
  return buf;
}


int
sa_deserialize_running_stats(sa_running_stats *s, const char *buf, size_t len)
{
  size_t elen = sizeof(double) * 3;
  if (len != elen) {
    sa_init_running_stats(s);
    return 1;
  }
  b2n(buf, &s->count, sizeof(double));
  b2n(buf + sizeof(double), &s->mean, sizeof(double));
  b2n(buf + sizeof(double) * 2, &s->sum, sizeof(double));
  if (s->count < 0) {
    sa_init_running_stats(s);
    return 2;
  }
  return 0;
}
