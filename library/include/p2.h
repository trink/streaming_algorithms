/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**  Piece_wise parabolic predicition (P2).
 *   Dynamic calculation of quantiles and histograms without storing
 *   observations. http://www.cs.wustl.edu/~jain/papers/ftp/psqr.pdf
 *  @file */

#ifndef sa_p2_h_
#define sa_p2_h_

#include <stddef.h>

typedef struct sa_p2_quantile sa_p2_quantile;
typedef struct sa_p2_histogram sa_p2_histogram;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Allocates and initializes the data structure.
 *
 * @param p p_quantile to calculate (e.g. 0.5 == median)
 */
sa_p2_quantile* sa_create_p2_quantile(double p);

/**
 * Zeros out the quantile.
 *
 * @param p2q Quantile struct
 */
void sa_init_p2_quantile(sa_p2_quantile *p2q);

/**
 * Updates the quantile with the provided observation.
 *
 * @param p2q Quantile struct
 * @param x Observation to add
 * @return p_quantile estimate
 *
 */
double
sa_add_p2_quantile(sa_p2_quantile *p2q, double x);

/**
 * Returns the number of observations that are less than or equal to the marker.
 *
 * @param p2q Quantile struct
 * @param marker Selects the percentile
 * 0 = min
 * 1 = p/2
 * 2 = p
 * 3 = (1+p)/2
 * 4 = max
 */
unsigned long long
sa_count_p2_quantile(sa_p2_quantile *p2q, unsigned short marker);

/**
 * Returns the estimated quantile value for the specified marker.
 *
 * @param p2q Quantile struct
 * @param marker 0-4 see sa_count_p2_quantile
 */
double
sa_estimate_p2_quantile(sa_p2_quantile *p2q, unsigned short marker);

/**
 * Free the associated memory.
 *
 * @param p2q Quantile struct
 *
 */
void sa_destroy_p2_quantile(sa_p2_quantile *p2q);

/**
 * Serialize the internal state to a buffer.
 *
 * @param p2q Quantile struct
 * @param len Length of the returned buffer
 *
 * @return char* Serialized representation MUST be freed by the caller
 */
char* sa_serialize_p2_quantile(sa_p2_quantile *p2q, size_t *len);

/**
 * Restores the internal state from the serialized output.
 *
 * @param p2q Quantile struct
 * @param buf Buffer containing the output of serialize_p2_quantile
 * @param len Length of the buffer
 *
 * @return 0 = success
 * 1 = invalid buffer length
 * 2 = invalid cnt
 * 3 = mis-matched percentile
 *
 */
int
sa_deserialize_p2_quantile(sa_p2_quantile *p2q, const char *buf, size_t len);

/**
 * Allocates and initializes the data structures.
 *
 * @param buckets Number of histogram buckets
 *
 */
sa_p2_histogram* sa_create_p2_histogram(unsigned short buckets);

/**
 * Zeros out the histogram counters.
 *
 * @param p2h Histogram struct
 */
void sa_init_p2_histogram(sa_p2_histogram *p2h);

/**
 * Updates the histogram with the provided observation.
 *
 * @param p2h Histogram struct
 * @param x Observation to add
 */
void sa_add_p2_histogram(sa_p2_histogram *p2h, double x);

/**
 * Gets the number of observations that are less than or equal to the marker.
 * *
 * @param p2h Histogram struct
 * @param marker Selects the percentile (marker/buckets)
 */
unsigned long long
sa_count_p2_histogram(sa_p2_histogram *p2h, unsigned short marker);

/**
 * Gets the estimated quantile value for the specified marker.
 *
 * @param p2h Histogram struct
 * @param marker Selects the percentile (marker/buckets)
*/
double
sa_estimate_p2_histogram(sa_p2_histogram *p2h, unsigned short marker);

/**
 * Free the associated memory.
 *
 * @param p2h Histogram struct
 *
 */
void sa_destroy_p2_histogram(sa_p2_histogram *p2h);

/**
 * Serialize the internal state to a buffer.
 *
 * @param p2h Histogram struct
 * @param len Length of the returned buffer
 *
 * @return char* Serialized representation MUST be freed by the caller
 */
char* sa_serialize_p2_histogram(sa_p2_histogram *p2h, size_t *len);

/**
 * Restores the internal state from the serialized output.
 *
 * @param p2h Histogram struct
 * @param buf Buffer containing the output of serialize_p2_histogram
 * @param len Length of the buffer
 *
 * @return 0 = success
 * 1 = invalid buffer length
 * 2 = invalid cnt
 *
 */
int
sa_deserialize_p2_histogram(sa_p2_histogram *p2h, const char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
