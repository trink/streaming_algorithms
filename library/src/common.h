/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**  Common utility functions @file */

#ifndef sa_common_h_
#define sa_common_h_

#include <stddef.h>

/**
 * Copies a number into a buffer as a little endian representation.
 *
 * @param n Pointer to the number
 * @param buf Output buffer MUST be at least 'len' bytes
 * @param len Number of bytes
 */
void n2b(const void *n, char *buf, size_t len);

/**
 * Copies a buffer with a little endian numeric representation into a number.
 *
 * @param buf Input buffer MUST be at least 'len' bytes
 * @param n Pointer to a number
 * @param len Number of bytes
 */
void b2n(const char *buf, void *n, size_t len);

#endif
