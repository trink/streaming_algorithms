/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Calculates the running count, mean, variance, and standard deviation
 *  @file */

#ifndef sa_running_stats_h_
#define sa_running_stats_h_

#include <stddef.h>

typedef struct sa_running_stats
{
  double count;
  double mean;
  double sum;
} sa_running_stats;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Zeros out the stats counters.
 *
 * @param s Stat structure to zero out
 */
void sa_init_running_stats(sa_running_stats *s);

/**
 * Value to add to the running stats.
 *
 * @param s Stat structure
 * @param d Value to add
 */
void sa_add_running_stats(sa_running_stats *s, double d);

/**
 * Returns the variance of the stats.
 *
 * @param s Stat structure
 *
 * @return double Variance of the stats up to this point
 */
double sa_variance_running_stats(sa_running_stats *s);

/**
 * Returns the corrected sample standard deviation of the stats.
 *
 * @param s Stat structure
 *
 * @return double Standard deviation of the stats up to this point
 */
double sa_sd_running_stats(sa_running_stats *s);

/**
 * Returns the uncorrected sample standard deviation of the stats.
 *
 * @param s Stat structure
 *
 * @return double Uncorrected standard deviation of the stats up to this point
 */
double sa_usd_running_stats(sa_running_stats *s);

/**
 * Serialize the internal state to a buffer.
 *
 * @param s Stat structure
 * @param len Length of the returned buffer
 *
 * @return char* Serialized representation MUST be freed by the caller
 */
char* sa_serialize_running_stats(sa_running_stats *s, size_t *len);

/**
 * Restores the internal state from the serialized output.
 *
 * @param s Stat structure
 * @param buf Buffer containing the output of serialize_running_stats
 * @param len Length of the buffer
 *
 * @return 0 = success
 * 1 = invalid buffer length
 * 2 = invalid count value
 *
 */
int
sa_deserialize_running_stats(sa_running_stats *s, const char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
