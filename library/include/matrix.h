/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**  Matrix data storage @file */

#ifndef sa_matrix_h_
#define sa_matrix_h_

#include <stddef.h>
#include <stdint.h>

typedef struct sa_matrix_int sa_matrix_int;
typedef struct sa_matrix_flt sa_matrix_flt;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Allocates and initializes the data structure.
 *
 * @param rows
 * @param cols
 *
 * @return Pointer to matrix_int
 *
 */
sa_matrix_int* sa_create_matrix_int(int rows, int cols);
sa_matrix_flt* sa_create_matrix_flt(int rows, int cols);

/**
 * Zeros out the matrix.
 * - matrix_int is set to all zeros
 * - matrix_flt is ste to all NAN
 *
 * @param m Pointer to matrix_int
 */
void sa_init_matrix_int(sa_matrix_int *m);
void sa_init_matrix_flt(sa_matrix_flt *m);

/**
 * Zeros out the specified matrix row.
 * - matrix_int is set to all zeros
 * - matrix_flt is ste to all NAN
 *
 * @param m Pointer to matrix_int
 * @param row
 *
 */
void sa_init_matrix_row_int(sa_matrix_int *m, int row);
void sa_init_matrix_row_flt(sa_matrix_flt *m, int row);

/**
 * Adds the specified value to the matrix.
 *
 * @param m Pointer to matrix_int
 * @param row
 * @param col
 * @param v Value to add
 *
 * @return current value
 *
 */
int sa_add_matrix_int(sa_matrix_int *m, int row, int col, int v);
float sa_add_matrix_flt(sa_matrix_flt *m, int row, int col, float v);

/**
 * Sets the time series row to the specified value.
 *
 * @param m Pointer to matrix_int
 * @param row
 * @param col
 * @param v Value to set
 *
 * @return current value
 *
 */
int sa_set_matrix_int(sa_matrix_int *m, int row, int col, int v);
float sa_set_matrix_flt(sa_matrix_flt *m, int row, int col, float v);


/**
 * Gets the value of the time series row.
 *
 * @param m Pointer to matrix_int
 * @param row
 * @param cols
 *
 * @return current value
 *
 */
int sa_get_matrix_int(sa_matrix_int *m, int row, int cols);
float sa_get_matrix_flt(sa_matrix_flt *m, int row, int cols);


/**
 * Free the associated memory.
 *
 * @param m Pointer to matrix_int
 *
 */
void sa_destroy_matrix_int(sa_matrix_int *m);
void sa_destroy_matrix_flt(sa_matrix_flt *m);

/**
 * Serialize the internal state to a buffer.
 *
 * @param m Pointer to matrix_int
 * @param len Length of the returned buffer
 *
 * @return char* Serialized representation MUST be freed by the caller
 */
char* sa_serialize_matrix_int(sa_matrix_int *m, size_t *len);
char* sa_serialize_matrix_flt(sa_matrix_flt *m, size_t *len);

/**
 * Restores the internal state from the serialized output.
 *
 * @param m Pointer to matrix_int
 * @param buf Buffer containing the output of serialize_matrix_int
 * @param len Length of the buffer
 *
 * @return 0 = success
 * 1 = invalid buffer length
 * 2 = invalid rows
 * 3 = invalid cols
 *
 */
int
sa_deserialize_matrix_int(sa_matrix_int *m,
                          const char *buf,
                          size_t len);
int
sa_deserialize_matrix_flt(sa_matrix_flt *m,
                          const char *buf,
                          size_t len);

#ifdef __cplusplus
}
#endif

#endif
