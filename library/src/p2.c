/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Piece_wise parabolic predicition implementation @file */

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

#include "common.h"
#include "p2_impl.h"

static int compare_double(const void *a, const void *b)
{
  if (*(double *)a < *(double *)b) {return -1;}
  if (*(double *)a == *(double *)b) {return 0;}
  return 1;
}


static double parabolic(int i, double d, double *q, double *n)
{
  return q[i] + d / (n[i + 1] - n[i - 1]) *
         (
          (n[i] - n[i - 1] + d) *
          (q[i + 1] - q[i]) / (n[i + 1] - n[i]) +
          (n[i + 1] - n[i] - d) *
          (q[i] - q[i - 1]) / (n[i] - n[i - 1])
         );
}


static double linear(int i, double d, double *q, double *n)
{
  return q[i] + d * (q[i + (int)d] - q[i]) / (n[i + (int)d] - n[i]);
}


sa_p2_quantile* sa_create_p2_quantile(double p)
{
  if (p < 0 || p > 1) {return NULL;}

  sa_p2_quantile *p2q = malloc(sizeof(*p2q));
  if (!p2q) {return NULL;}

  p2q->p = (float)p;
  sa_init_p2_quantile(p2q);
  return p2q;
}


void sa_destroy_p2_quantile(sa_p2_quantile *p2q)
{
  free(p2q);
}


void sa_init_p2_quantile(sa_p2_quantile *p2q)
{
  assert(p2q);

  p2q->cnt  = QUANTILE_MARKERS;
  p2q->q[0] = 0;
  p2q->q[1] = 0;
  p2q->q[2] = 0;
  p2q->q[3] = 0;
  p2q->q[4] = 0;

  p2q->n[0] = 1;
  p2q->n[1] = 2;
  p2q->n[2] = 3;
  p2q->n[3] = 4;
  p2q->n[4] = 5;

  p2q->n1[0] = 1;
  p2q->n1[1] = 1 + 2 * p2q->p;
  p2q->n1[2] = 1 + 4 * p2q->p;
  p2q->n1[3] = 3 + 2 * p2q->p;
  p2q->n1[4] = 5;
}


double sa_add_p2_quantile(sa_p2_quantile *p2q, double x)
{
  assert(p2q);

  if (p2q->cnt) {
    p2q->q[--p2q->cnt] = x;
    if (p2q->cnt == 0) {
      qsort(p2q->q, QUANTILE_MARKERS, sizeof(double), compare_double);
      return p2q->n[2];
    }
    return NAN;
  }

  double *q = p2q->q;
  double *n = p2q->n;
  int k = 0;
  if (x < q[0]) {
    q[0] = x;
    k = 1;
  } else if (q[0] <= x && x < q[1]) {
    k = 1;
  } else if (q[1] <= x && x < q[2]) {
    k = 2;
  } else if (q[2] <= x && x < q[3]) {
    k = 3;
  } else if (q[3] <= x && x <= q[4]) {
    k = 4;
  } else if (q[4] < x) {
    q[4] = x;
    k = 4;
  }

  for (int i = k; i < QUANTILE_MARKERS; ++i) {
    ++n[i];
  }

  p2q->n1[1] += p2q->p / 2;
  p2q->n1[2] += p2q->p;
  p2q->n1[3] += (1 + p2q->p) / 2;
  ++p2q->n1[4];

  for (int i = 1; i < QUANTILE_MARKERS - 1; ++i) {
    double d = p2q->n1[i] - n[i];
    if ((d >= 1 && n[i + 1] - n[i] > 1) || (d <= -1 && n[i - 1] - n[i] < -1)) {
      d = (d > 0) ? 1 : -1;
      double q1 = parabolic(i, d, q, n);
      if (q[i - 1] < q1 && q1 < q[i + 1]) {
        q[i] = q1;
      } else {
        q[i] = linear(i, d, q, n);
      }
      n[i] += d;
    }
  }
  return q[2];
}


double sa_estimate_p2_quantile(sa_p2_quantile *p2q, unsigned short marker)
{
  assert(p2q);

  if (marker >= QUANTILE_MARKERS || p2q->cnt != 0) return NAN;
  return p2q->q[marker];
}


unsigned long long
sa_count_p2_quantile(sa_p2_quantile *p2q, unsigned short marker)
{
  assert(p2q);

  if (marker >= QUANTILE_MARKERS || p2q->cnt != 0) return 0;
  return (unsigned long long)p2q->n[marker];
}


static size_t quantile_size()
{
  return sizeof(unsigned short) + sizeof(float) + sizeof(double)
         * QUANTILE_MARKERS * 3;
}



char* sa_serialize_p2_quantile(sa_p2_quantile *p2q, size_t *len)
{
  assert(p2q && len);

  *len = quantile_size();
  char *buf = malloc(*len);
  if (!buf) {
    *len = 0;
    return NULL;
  }

  char *cp = buf;
  n2b(&p2q->cnt, cp, sizeof(unsigned short));
  cp += sizeof(unsigned short);

  n2b(&p2q->p, cp, sizeof(float));
  cp += sizeof(float);

  for (unsigned i = 0; i < QUANTILE_MARKERS; ++i, cp += sizeof(double)) {
    n2b(p2q->q + i, cp, sizeof(double));
  }

  for (unsigned i = 0; i < QUANTILE_MARKERS; ++i, cp += sizeof(double)) {
    n2b(p2q->n + i, cp, sizeof(double));
  }

  for (unsigned i = 0; i < QUANTILE_MARKERS; ++i, cp += sizeof(double)) {
    n2b(p2q->n1 + i, cp, sizeof(double));
  }
  return buf;
}


int
sa_deserialize_p2_quantile(sa_p2_quantile *p2q, const char *buf, size_t len)
{
  assert(p2q && buf);

  size_t elen =  quantile_size();
  if (len != elen) {
    sa_init_p2_quantile(p2q);
    return 1;
  }

  const char *cp = buf;
  b2n(cp, &p2q->cnt, sizeof(unsigned short));
  if (p2q->cnt > QUANTILE_MARKERS) {
    sa_init_p2_quantile(p2q);
    return 2;
  }
  cp += sizeof(unsigned short);

  float p;
  b2n(cp, &p, sizeof(float));
  if (p != p2q->p) {
    sa_init_p2_quantile(p2q);
    return 3;
  }
  cp += sizeof(float);

  for (unsigned i = 0; i < QUANTILE_MARKERS; ++i, cp += sizeof(double)) {
    b2n(cp, p2q->q + i, sizeof(double));
  }

  for (unsigned i = 0; i < QUANTILE_MARKERS; ++i, cp += sizeof(double)) {
    b2n(cp, p2q->n + i, sizeof(double));
  }

  for (unsigned i = 0; i < QUANTILE_MARKERS; ++i, cp += sizeof(double)) {
    b2n(cp, p2q->n1 + i, sizeof(double));
  }
  return 0;
}


sa_p2_histogram* sa_create_p2_histogram(unsigned short buckets)
{
  if (buckets < 4 || buckets > USHRT_MAX - 1) {return NULL;}

  sa_p2_histogram *p2h = malloc(sizeof(*p2h) + sizeof(double) * (buckets + 1)
                                * 2);
  if (!p2h) {return NULL;}

  p2h->b = buckets;
  sa_init_p2_histogram(p2h);
  return p2h;
}


void sa_init_p2_histogram(sa_p2_histogram *p2h)
{
  assert(p2h);

  p2h->cnt = p2h->b + 1;
  for (unsigned short i = 0; i <= p2h->b; ++i) {
    p2h->data[i] = 0;
  }

  double *n = p2h->data + p2h->b + 1;
  for (unsigned short i = 0; i <= p2h->b; ++i) {
    n[i] = i + 1;
  }
}


void sa_add_p2_histogram(sa_p2_histogram *p2h, double x)
{
  assert(p2h);

  if (p2h->cnt) {
    p2h->data[--p2h->cnt] = x;
    if (p2h->cnt == 0) {
      qsort(p2h->data, p2h->b + 1, sizeof(double), compare_double);
    }
    return;
  }

  double *q = p2h->data;
  double *n = p2h->data + p2h->b + 1;
  int k = 0;
  if (x < q[0]) {
    q[0] = x;
    k = 1;
  } else {
    for (unsigned short i = 0; i < p2h->b - 1; ++i) {
      if (q[i] <= x && x < q[i + 1]) {
        k = i + 1;
        break;
      }
    }
  }
  if (k == 0) {
    if (q[p2h->b - 1] <= x && x <= q[p2h->b]) {
      k = p2h->b;
    } else if (q[p2h->b] < x) {
      q[p2h->b] = x;
      k = p2h->b;
    }
  }

  for (unsigned short i = k; i <= p2h->b; ++i) {
    ++n[i];
  }

  for (unsigned short i = 1; i < p2h->b; ++i) {
    double n1 = 1 + i * (n[p2h->b] - 1) / p2h->b;
    double d = n1 - n[i];
    if ((d >= 1 && n[i + 1] - n[i] > 1) || (d <= -1 && n[i - 1] - n[i] < -1)) {
      d = (d > 0) ? 1 : -1;
      double q1 = parabolic(i, d, q, n);
      if (q[i - 1] < q1 && q1 < q[i + 1]) {
        q[i] = q1;
      } else {
        q[i] = linear(i, d, q, n);
      }
      n[i] += d;
    }
  }
}


double sa_estimate_p2_histogram(sa_p2_histogram *p2h, unsigned short marker)
{
  assert(p2h);

  if (marker > p2h->b || p2h->cnt != 0) return NAN;
  return p2h->data[marker];
}


unsigned long long
sa_count_p2_histogram(sa_p2_histogram *p2h, unsigned short marker)
{
  assert(p2h);

  if (marker > p2h->b || p2h->cnt != 0) return 0;
  return (unsigned long long)p2h->data[p2h->b + 1 + marker];
}


void sa_destroy_p2_histogram(sa_p2_histogram *p2h)
{
  free(p2h);
}


static size_t histogram_size(sa_p2_histogram *p2h)
{
  return sizeof(unsigned short) + sizeof(double) * (p2h->b + 1U) * 2;
}


char* sa_serialize_p2_histogram(sa_p2_histogram *p2h, size_t *len)
{
  assert(p2h && len);

  *len = histogram_size(p2h);
  char *buf = malloc(*len);
  if (!buf) {
    *len = 0;
    return NULL;
  }

  char *cp = buf;
  n2b(&p2h->cnt, cp, sizeof(unsigned short));
  cp += sizeof(unsigned short);
  for (unsigned i = 0; i < (p2h->b + 1U) * 2; ++i, cp += sizeof(double)) {
    n2b(p2h->data + i, cp, sizeof(double));
  }
  return buf;
}


int
sa_deserialize_p2_histogram(sa_p2_histogram *p2h, const char *buf, size_t len)
{
  assert(p2h && buf);

  size_t elen = histogram_size(p2h);
  if (len != elen) {
    sa_init_p2_histogram(p2h);
    return 1;
  }

  const char *cp = buf;
  b2n(cp, &p2h->cnt, sizeof(unsigned short));
  if (p2h->cnt > p2h->b + 1) {
    sa_init_p2_histogram(p2h);
    return 2;
  }
  cp += sizeof(unsigned short);
  for (unsigned i = 0; i < (p2h->b + 1U) * 2; ++i, cp += sizeof(double)) {
    b2n(cp, p2h->data + i, sizeof(double));
  }
  return 0;
}
