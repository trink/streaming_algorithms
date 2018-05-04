/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**  Time series data storage (stripped down circular buffer) @file */

#ifndef sa_time_series_h_
#define sa_time_series_h_

#include <stddef.h>
#include <stdint.h>

typedef struct sa_time_series_int sa_time_series_int;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Allocates and initializes the data structure.
 *
 * @param rows Number of observation slots
 * @param ns_per_row Nanoseconds represented in each row
 *
 * @return Pointer to time_series_int
 *
 */
sa_time_series_int* sa_create_time_series_int(int rows, uint64_t ns_per_row);

/**
 * Zeros out the time series.
 *
 * @param ts Pointer to time_series_int
 */
void sa_init_time_series_int(sa_time_series_int *ts);

/**
 * Adds the specified value to the time series row.
 *
 * @param ts Pointer to time_series_int
 * @param ns Timestamp (nanoseconds since Jan 1 1970) associated with value
 * @param v Value to add
 *
 * @return current value
 *
 */
int
sa_add_time_series_int(sa_time_series_int *ts, uint64_t ns, int v);

/**
 * Sets the time series row to the specified value.
 *
 * @param ts Pointer to time_series_int
 * @param ns Timestamp (nanoseconds since Jan 1 1970) associated with value
 * @param v Value to set
 *
 * @return current value
 *
 */
int
sa_set_time_series_int(sa_time_series_int *time_series_int, uint64_t ns, int v);

/**
 * Gets the value of the time series row.
 *
 * @param ts Pointer to time_series_int
 * @param ns Timestamp (nanoseconds since Jan 1 1970) of the row
 *
 * @return current value
 *
 */
int
sa_get_time_series_int(sa_time_series_int *time_series_int, uint64_t ns);

/**
 * Returns the timestamp of the most recent row.
 *
 * @param ts Pointer to time_series_int
 *
 * @return Timestamp (nanoseconds since Jan 1 1970)
 *
 */
uint64_t sa_timestamp_time_series_int(sa_time_series_int *ts);

/**
 * Computes the matrix profile for a time series or sub set of the series. See:
 * http://www.cs.ucr.edu/~eamonn/MatrixProfile.html
 *
 * @param ts Pointer to time_series_int
 * @param ns The start of the interval to analyze
 * @param n Sequence length
 * @param m Sub-sequence length
 * @param percent Percentage of data to base the calculation on (0.0 <
 *                percent <= 100). Use less than 100 to produce an estimate
 *                of the matrix profile trading accuracy for speed.
 * @param mp Returned pointer to the matrix profile array (MUST be freed by the
 *           caller)
 * @param mpi Returned pointer to the matrix profile index array (MUST be freed
 *            by the caller)
 * @return int Length of the returned matrix profile arrays (0 on failure). The
 *         mp/i pointers are not modified on failure.
 */
int sa_mp_time_series_int(sa_time_series_int *ts,
                          uint64_t ns,
                          int n,
                          int m,
                          double percent,
                          double *mp[],
                          int *mpi[]);

/**
 * Free the associated memory.
 *
 * @param ts Pointer to time_series_int
 *
 */
void sa_destroy_time_series_int(sa_time_series_int *ts);

/**
 * Serialize the internal state to a buffer.
 *
 * @param ts Pointer to time_series_int
 * @param len Length of the returned buffer
 *
 * @return char* Serialized representation MUST be freed by the caller
 */
char* sa_serialize_time_series_int(sa_time_series_int *ts, size_t *len);

/**
 * Restores the internal state from the serialized output.
 *
 * @param ts Pointer to time_series_int
 * @param buf Buffer containing the output of serialize_time_series_int
 * @param len Length of the buffer
 *
 * @return 0 = success
 * 1 = invalid buffer length
 * 2 = invalid cnt
 * 3 = mis-matched dimensions
 *
 */
int
sa_deserialize_time_series_int(sa_time_series_int *ts,
                               const char *buf,
                               size_t len);

#ifdef __cplusplus
}
#endif

#endif
