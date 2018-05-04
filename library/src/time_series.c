/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief time_series implementation @file */

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "running_stats.h"
#include "time_series_impl.h"

struct mp_calc {
  double  *stats;
  double  *dp;
  double  *mp;
  int     *mpi;
  int     *rand;
  int     rand_len;
  int     mp_len;
  int     n;
  int     m;
  int     sidx;
};


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


static void shuffle_idx(int *a, int len)
{
  for (int i = 0; i < len - 1; ++i) {
    int j = i + rand() / (RAND_MAX / (len - i) + 1);
    int tmp = a[j];
    a[j] = a[i];
    a[i] = tmp;
  }
}


static bool init_calc(struct mp_calc *c, int n, int m)
{
  c->n = n;
  c->m = m;
  c->mp_len = n - m + 1;
  c->rand = NULL;
  c->mpi = NULL;
  c->mp = NULL;
  c->dp = NULL;

  c->stats = malloc(sizeof(double) * 2 * c->mp_len);
  if (!c->stats) {return false;}

  c->dp = malloc(sizeof(double) * c->n);
  if (!c->dp) {return false;}

  c->mp = malloc(sizeof(double) * c->mp_len);
  if (!c->mp) {return false;}

  c->mpi = calloc(c->mp_len, sizeof(int));
  if (!c->mpi) {return false;}

  int exclude = m / 4;
  c->rand_len = c->mp_len - exclude - 1;
  c->rand = malloc(sizeof(int) * c->rand_len);
  if (!c->rand) {return false;}

  for (int i = 0; i < c->mp_len; ++i) {
    c->mp[i] = INFINITY;
  }

  for (int i = 0, idx = exclude + 1; idx < c->mp_len; ++i, ++idx) {
    c->rand[i] = idx;
  }
  shuffle_idx(c->rand, c->rand_len);
  return true;
}


static int tsidx(int j, int n, int offset)
{
  int idx = j + offset;
  return idx >= n ? idx - n : idx;
}


static void compute_stats(sa_time_series_int *ts, struct mp_calc *c)
{
  sa_running_stats rs;
  sa_init_running_stats(&rs);
  int window = 0;
  int idx = c->sidx;
  for (int i = 0; i < c->n; ++i, ++idx) {
    if (idx == ts->rows) {idx = 0;}
    if (i >= c->m) {
      c->stats[window * 2] = rs.mean;
      c->stats[window * 2 + 1] = sa_usd_running_stats(&rs);
      ++window;
      int fi = idx - c->m;
      if (fi < 0) {
        fi += ts->rows;
      }
      double pm = rs.mean;
      rs.mean += (ts->v[idx] - ts->v[fi]) / rs.count;
      rs.sum += (ts->v[idx] - pm) * (ts->v[idx] - rs.mean) -
          (ts->v[fi] - pm) * (ts->v[fi] - rs.mean);
    } else {
      sa_add_running_stats(&rs, ts->v[idx]);
    }
  }
  c->stats[window * 2] = rs.mean;
  c->stats[window * 2 + 1] = sa_usd_running_stats(&rs);
}


static void scrimp_int(sa_time_series_int *ts, struct mp_calc *c, int stop)
{
  compute_stats(ts, c);
  int j, i;
  double d, lastz;
  for (int ri = 0; ri < c->rand_len; ++ri) {
    int diag = c->rand[ri];
    for (j = diag; j < c->n; ++j) {
      c->dp[j] = ts->v[tsidx(j, ts->rows, c->sidx)]
          * ts->v[tsidx(j - diag, ts->rows, c->sidx)];
    }

    // evaluate the first distance value in the current diagonal
    lastz = 0;
    for (j = 0; j < c->m; j++) {
      lastz += c->dp[j + diag];
    }

    j = diag, i = 0;
    d = 2 * (c->m - (lastz - c->m * c->stats[j * 2] * c->stats[i * 2])
             / (c->stats[j * 2 + 1] * c->stats[i * 2 + 1]));
    if (d < c->mp[j]) {
      c->mp[j] = d;
      c->mpi[j] = i;
    }
    if (d < c->mp[i]) {
      c->mp[i] = d;
      c->mpi[i] = j;
    }

    // evaluate the remaining distance values in the current diagonal
    for (j = diag + 1; j < c->mp_len; ++j) {
      i = j - diag;
      lastz = lastz + c->dp[j + c->m - 1] - c->dp[j - 1];
      d = 2 * (c->m - (lastz - c->m * c->stats[j * 2] * c->stats[i * 2])
               / (c->stats[j * 2 + 1] * c->stats[i * 2 + 1]));
      if (d < c->mp[j]) {
        c->mp[j] = d;
        c->mpi[j] = i;
      }
      if (d < c->mp[i]) {
        c->mp[i] = d;
        c->mpi[i] = j;
      }
    }
    if (ri == stop) {
      break;
    }
  }

  // convert to distances
  for (i = 0; i < c->mp_len; ++i) {
    c->mp[i] = sqrt(fabs(c->mp[i]));
  }
}


int sa_mp_time_series_int(sa_time_series_int *ts,
                          uint64_t ns,
                          int n,
                          int m,
                          double percent,
                          double *mp[],
                          int *mpi[])
{
  int idx = find_index_int(ts, ns, false);
  if (idx == -1 || n > ts->rows || percent <= 0 || percent > 100
      || m < 4 || n < 4 * m || n % m != 0) {
    return 0;
  }

  struct mp_calc c;
  c.sidx = idx;
  if (!init_calc(&c, n, m)) {
    free(c.stats);
    free(c.dp);
    free(c.rand);
    free(c.mp);
    free(c.mpi);
    return 0;
  }
  scrimp_int(ts, &c, percent / 100 * c.mp_len + 1);
  free(c.stats);
  free(c.dp);
  free(c.rand);
  *mp = c.mp;
  *mpi = c.mpi;
  return c.mp_len;
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
