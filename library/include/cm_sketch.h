/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**  Count-min sketch https://en.wikipedia.org/wiki/Count%E2%80%93min_sketch
 *   @file */

#ifndef sa_cm_sketch_h_
#define sa_cm_sketch_h_

#include <stdint.h>

typedef struct sa_cm_sketch sa_cm_sketch;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Allocate and initialize the data structure.
 *
 * @param epsilon Approximation factor
 *
 * @param delta Probability of failure
 *
 * @return Count-min sketch struct
 *
 */
sa_cm_sketch* sa_create_cms(double epsilon, double delta);

/**
 * Zero out the data structure.
 *
 * @param cms Count-min sketch struct
 */
void sa_init_cms(sa_cm_sketch *cms);

/**
 * Free the associated memory.
 *
 * @param cms Count-min sketch struct
 *
 */
void sa_destroy_cms(sa_cm_sketch *cms);

/**
 * Point query the frequency count of item.
 *
 * @param cms Count-min sketch struct
 * @param item Item to query
 * @param len Length of the item in bytes
 *
 * @return int Estimated count
 */
uint32_t sa_point_query_cms(sa_cm_sketch *cms, void *item, size_t len);

/**
 * Increment/Decrement the Count-min sketch with the specified item and value.
 *
 * @param cms Count-min sketch struct
 * @param item Item to add
 * @param len Length of the item in bytes
 * @param n Number of items to add/remove
 *
 * @return int Estimated count
 */
uint32_t sa_update_cms(sa_cm_sketch *cms, void *item, size_t len, int n);

/**
 * Return the total number of items added to the sketch.
 *
 * @param cms Count-min sketch struct
 *
 * @return size_t Number of items added to the sketch
 */
uint64_t sa_item_count_cms(sa_cm_sketch *cms);

/**
 * Return the total number of unique items added to the sketch.
 *
 * @param cms Count-min sketch struct
 *
 * @return size_t Number of unique items added to the sketch
 */
uint64_t sa_unique_count_cms(sa_cm_sketch *cms);

/**
 * Serialize the internal state to a buffer.
 *
 * @param cms Count-min sketch struct
 * @param len Length of the returned buffer
 *
 * @return char* Serialized representation MUST be freed by the caller
 */
char* sa_serialize_cms(sa_cm_sketch *cms, size_t *len);

/**
 * Restore the internal state from the serialized output.
 *
 * @param cms Count-min sketch struct
 * @param buf Buffer containing the output of serialize_cms
 * @param len Length of the buffer
 *
 * @return 0 = success
 * 1 = invalid buffer length
 *
 */
int sa_deserialize_cms(sa_cm_sketch *cms, const char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
