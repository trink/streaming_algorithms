/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief time_series implementation @file */

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "time_series_impl.h"


static int find_index_int(sa_time_series_int *ts, uint64_t ns, bool advance)
{
  int64_t current_row = ts->current_time / ts->ns_per_row;
  int64_t requested_row = ns / ts->ns_per_row;
  int64_t row_delta = requested_row - current_row;

  if (row_delta > 0 && advance) {
    if (row_delta >= ts->rows) {
      memset(ts->v, 0, sizeof(int) * ts->rows);
    } else {
      int oidx = current_row % ts->rows + 1;
      if (oidx == ts->rows) {oidx = 0;}
      if (oidx + row_delta <= ts->rows) {
        memset(ts->v + oidx, 0, sizeof(int) * row_delta);
      } else {
        memset(ts->v + oidx, 0, sizeof(int) * (ts->rows - oidx));
        memset(ts->v, 0, sizeof(int) * (oidx + row_delta - ts->rows));
      }
    }
    ts->current_time = ns - (ns % ts->ns_per_row);
  } else if (requested_row > current_row || abs(row_delta) >= ts->rows) {
    return -1;
  }
  return requested_row % ts->rows;
}


sa_time_series_int* sa_create_time_series_int(int rows, uint64_t ns_per_row)
{
  if (rows < 2 || ns_per_row < 1) {return NULL;}

  sa_time_series_int *ts = malloc(sizeof(*ts) + sizeof(int) * rows);
  if (!ts) {return NULL;}

  ts->ns_per_row = ns_per_row;
  ts->rows = rows;
  sa_init_time_series_int(ts);
  return ts;
}


void sa_destroy_time_series_int(sa_time_series_int *ts)
{
  free(ts);
}


void sa_init_time_series_int(sa_time_series_int *ts)
{
  assert(ts);
  ts->current_time = ts->ns_per_row * (ts->rows - 1);
  memset(ts->v, 0, sizeof(int) * ts->rows);
}


int sa_add_time_series_int(sa_time_series_int *ts, uint64_t ns, int v)
{
  assert(ts);
  int idx = find_index_int(ts, ns, true);
  if (idx == -1) {return INT_MIN;}
  int64_t nv = (int64_t)ts->v[idx] + v;
  if (nv > INT_MAX) {
    nv = INT_MAX;
  } else if (nv < INT_MIN) {
    nv = INT_MIN;
  }
  ts->v[idx] = nv;
  return nv;
}


int sa_set_time_series_int(sa_time_series_int *ts, uint64_t ns, int v)
{
  assert(ts);
  int idx = find_index_int(ts, ns, true);
  if (idx == -1) {return INT_MIN;}
  ts->v[idx] = v;
  return v;
}


int sa_get_time_series_int(sa_time_series_int *ts, uint64_t ns)
{
  assert(ts);
  int idx = find_index_int(ts, ns, false);
  if (idx == -1) {return INT_MIN;}
  return ts->v[idx];
}


uint64_t sa_timestamp_time_series_int(sa_time_series_int *ts)
{
  assert(ts);
  return ts->current_time;
}


static size_t time_series_int_size(sa_time_series_int *ts)
{
  return sizeof(sa_time_series_int) + sizeof(int) * ts->rows;
}


char* sa_serialize_time_series_int(sa_time_series_int *ts, size_t *len)
{
  assert(ts && len);

  *len = time_series_int_size(ts);
  char *buf = malloc(*len);
  if (!buf) {
    *len = 0;
    return NULL;
  }

  char *cp = buf;
  n2b(&ts->current_time, cp, sizeof(uint64_t));
  cp += sizeof(uint64_t);

  n2b(&ts->ns_per_row, cp, sizeof(uint64_t));
  cp += sizeof(uint64_t);

  n2b(&ts->rows, cp, sizeof(int));
  cp += sizeof(int);

  for (int i = 0; i < ts->rows; ++i, cp += sizeof(int)) {
    n2b(ts->v + i, cp, sizeof(int));
  }
  return buf;
}


int
sa_deserialize_time_series_int(sa_time_series_int *ts, const char *buf,
                               size_t len)
{
  assert(ts && buf);

  size_t elen = time_series_int_size(ts);
  if (len != elen) {
    sa_init_time_series_int(ts);
    return 1;
  }

  const char *cp = buf;
  b2n(cp, &ts->current_time, sizeof(uint64_t));
  cp += sizeof(uint64_t);

  uint64_t ns_per_row;
  b2n(cp, &ns_per_row, sizeof(uint64_t));
  if (ns_per_row != ts->ns_per_row) {
    sa_init_time_series_int(ts);
    return 2;
  }
  cp += sizeof(uint64_t);

  int rows;
  b2n(cp, &rows, sizeof(int));
  if (rows != ts->rows) {
    sa_init_time_series_int(ts);
    return 3;
  }
  cp += sizeof(int);

  for (int i = 0; i < rows; ++i, cp += sizeof(int)) {
    b2n(cp, ts->v + i, sizeof(int));
  }

  return 0;
}
