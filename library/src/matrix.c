/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set m=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief matrix implementation @file */

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "running_stats.h"
#include "matrix_impl.h"


#define check_bounds(m, r, c)                                                  \
assert(m);                                                                     \
if (r < 0 || r >= m->rows || c < 0 || c >= m->cols) {                          \
  return INT_MIN;                                                              \
}


sa_matrix_int* sa_create_matrix_int(int rows, int cols)
{
  if (rows < 1 || cols < 1) {return NULL;}

  sa_matrix_int *m = malloc(sizeof(*m) + sizeof(int) * rows * cols);
  if (!m) {return NULL;}

  m->rows = rows;
  m->cols = cols;
  sa_init_matrix_int(m);
  return m;
}


void sa_init_matrix_row_int(sa_matrix_int *m, int row)
{
  assert(m);
  if (row < 0 || row >= m->rows) {return;};
  int64_t idx = (int64_t)row * m->cols;
  memset(m->v + idx, 0, sizeof(int) * m->cols);
}


void sa_destroy_matrix_int(sa_matrix_int *m)
{
  free(m);
}


void sa_init_matrix_int(sa_matrix_int *m)
{
  assert(m);
  memset(m->v, 0, sizeof(int) * m->rows * m->cols);
}


int sa_add_matrix_int(sa_matrix_int *m, int row, int col, int v)
{
  check_bounds(m, row, col);
  int64_t idx = (int64_t)row * m->cols + col;
  int64_t nv = (int64_t)m->v[idx] + v;
  if (nv > INT_MAX) {
    nv = INT_MAX;
  } else if (nv < INT_MIN) {
    nv = INT_MIN;
  }
  m->v[idx] = nv;
  return nv;
}


int sa_set_matrix_int(sa_matrix_int *m, int row, int col, int v)
{
  check_bounds(m, row, col);
  int64_t idx = (int64_t)row * m->cols + col;
  m->v[idx] = v;
  return v;
}


int sa_get_matrix_int(sa_matrix_int *m, int row, int col)
{
  check_bounds(m, row, col);
  int64_t idx = (int64_t)row * m->cols + col;
  return m->v[idx];
}


static size_t matrix_int_size(sa_matrix_int *m)
{
  return sizeof(sa_matrix_int) + sizeof(int) * m->rows * m->cols;
}


char* sa_serialize_matrix_int(sa_matrix_int *m, size_t *len)
{
  assert(m && len);

  *len = matrix_int_size(m);
  char *buf = malloc(*len);
  if (!buf) {
    *len = 0;
    return NULL;
  }

  char *cp = buf;

  n2b(&m->rows, cp, sizeof(int));
  cp += sizeof(int);

  n2b(&m->cols, cp, sizeof(int));
  cp += sizeof(int);

  for (int64_t i = 0; i < (int64_t)m->rows * m->cols; ++i, cp += sizeof(int)) {
    n2b(m->v + i, cp, sizeof(int));
  }
  return buf;
}


int
sa_deserialize_matrix_int(sa_matrix_int *m, const char *buf,
                          size_t len)
{
  assert(m && buf);

  size_t elen = matrix_int_size(m);
  if (len != elen) {
    sa_init_matrix_int(m);
    return 1;
  }

  const char *cp = buf;
  int rows;
  b2n(cp, &rows, sizeof(int));
  if (rows != m->rows) {
    sa_init_matrix_int(m);
    return 2;
  }
  cp += sizeof(int);

  int cols;
  b2n(cp, &cols, sizeof(int));
  if (cols != m->cols) {
    sa_init_matrix_int(m);
    return 3;
  }
  cp += sizeof(int);

  for (int64_t i = 0; i < (int64_t)rows * cols; ++i, cp += sizeof(int)) {
    b2n(cp, m->v + i, sizeof(int));
  }

  return 0;
}
