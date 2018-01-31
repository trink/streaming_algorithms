/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Count-min Sketch implementation @file */

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "cm_sketch_impl.h"
#include "xxhash.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

const double g_eulers_number = 2.718281828459045;

sa_cm_sketch* sa_create_cms(double epsilon, double delta)
{
  if (epsilon <= 0.0 || epsilon >= 1.0) {return NULL;}
  double w = ceil(g_eulers_number / epsilon);

  if (delta <= 0.0 || delta >= 1.0) {return NULL;}
  double d = ceil(log(1 / delta));

  double s = w * d;
  if (s > SIZE_MAX) {return NULL;}

  sa_cm_sketch *cms = malloc(sizeof(*cms) + sizeof(uint32_t) * (size_t)s);
  if (!cms) {return NULL;}

  cms->width = (uint32_t)w;
  cms->depth = (uint32_t)d;
  sa_init_cms(cms);
  return cms;
}


void sa_init_cms(sa_cm_sketch *cms)
{
  assert(cms);
  cms->item_count = 0;
  cms->unique_count = 0;
  memset(cms->counts, 0, sizeof(uint32_t) * cms->width * cms->depth);
}


void sa_destroy_cms(sa_cm_sketch *cms)
{
  free(cms);
}


uint32_t sa_point_query_cms(sa_cm_sketch *cms, void *item, size_t len)
{
  return sa_update_cms(cms, item, len, 0);
}


uint32_t
sa_update_cms(sa_cm_sketch *cms, void *item, size_t len, int n)
{
  assert(cms);
  uint32_t est = UINT32_MAX;
  // use enhanced double hashing (Kirsh & Mitzenmacher) to create the hash
  // value used for the width index
  XXH32_hash_t h1 = XXH32(item, len, 1);
  XXH32_hash_t h2 = XXH32(item, len, 2);

  for (uint32_t i = 0; i < cms->depth; ++i) {
    uint32_t d = i * cms->width;
    uint32_t w = (h1 + i * h2 + i * i) % cms->width;
    uint32_t cnt = cms->counts[d + w];
    est = MIN(est, cnt);
  }

  if (n > 0) { // add
    if (est == 0) {
      ++cms->unique_count;
    }

    int added = 0;
    for (uint32_t i = 0; i < cms->depth; ++i) {
      uint32_t d = i * cms->width;
      uint32_t w = (h1 + i * h2 + i * i) % cms->width;
      uint32_t cnt = cms->counts[d + w];
      // conservative update
      if (UINT32_MAX - cnt < (uint32_t)n) {
        uint32_t tmp = UINT32_MAX - cnt;
        cms->counts[d + w] = MAX(cnt, est + tmp);
        added = MAX((uint32_t)added, tmp);
      } else {
        cms->counts[d + w] = MAX(cnt, est + n);
        added = MAX(added, n);
      }
    }
    cms->item_count += added;
    return est + added;
  } else if (n < 0 && est != 0) { // remove
    n = abs(n);
    if ((uint32_t)n >= est) {
      n = est;
      --cms->unique_count;
    }

    for (uint32_t i = 0; i < cms->depth; ++i) {
      uint32_t d = i * cms->width;
      uint32_t w = (h1 + i * h2 + i * i) % cms->width;
      cms->counts[d + w] -= n;
    }
    cms->item_count -= n;
    return est - n;
  }
  return est;
}


uint64_t sa_item_count_cms(sa_cm_sketch *cms)
{
  assert(cms);
  return cms->item_count;
}


uint64_t sa_unique_count_cms(sa_cm_sketch *cms)
{
  assert(cms);
  return cms->unique_count;
}


static size_t serialized_size(sa_cm_sketch *cms)
{
  return sizeof(uint64_t) * 2  + sizeof(uint32_t) * cms->width * cms->depth;
}


char* sa_serialize_cms(sa_cm_sketch *cms, size_t *len)
{
  assert(cms && len);
  *len = serialized_size(cms);
  char *buf = malloc(*len);
  if (!buf) {
    *len = 0;
    return NULL;
  }

  char *cp = buf;
  n2b(&cms->item_count, cp, sizeof(uint64_t));
  cp += sizeof(uint64_t);
  n2b(&cms->unique_count, cp, sizeof(uint64_t));
  cp += sizeof(uint64_t);
  for (uint32_t i = 0; i < cms->width * cms->depth; ++i, cp += sizeof(uint32_t)) {
    n2b(cms->counts + i, cp, sizeof(uint32_t));
  }
  return buf;
}


int sa_deserialize_cms(sa_cm_sketch *cms, const char *buf, size_t len)
{
  assert(cms && buf);
  size_t elen = serialized_size(cms);
  if (len != elen) {
    sa_init_cms(cms);
    return 1;
  }

  const char *cp = buf;
  b2n(cp, &cms->item_count, sizeof(uint64_t));
  cp += sizeof(uint64_t);
  b2n(cp, &cms->unique_count, sizeof(uint64_t));
  cp += sizeof(uint64_t);
  for (uint32_t i = 0; i < cms->width * cms->depth; ++i, cp += sizeof(uint32_t)) {
    b2n(cp, cms->counts + i, sizeof(uint32_t));
  }
  return 0;
}
