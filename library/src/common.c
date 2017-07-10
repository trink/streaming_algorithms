/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Common utility function implementations @file */

#include <stdlib.h>
#include <string.h>

#include "common.h"

#ifdef IS_BIG_ENDIAN
void n2b(const void *n, char *buf, size_t len)
{
  char *p = (char *)n;
  for (int i = 0, j = len - 1; i < len; ++i, --j) {
    buf[i] = p[j];
  }
}

void b2n(const char *buf, void *n, size_t len)
{
  char *p = (char *)n;
  for (int i = 0, j = len - 1; i < len; ++i, --j) {
    p[i] = buf[j];
  }
}
#else
void n2b(const void *n, char *buf, size_t len)
{
  memcpy(buf, n, len);
}

void b2n(const char *buf, void *n, size_t len)
{
  memcpy(n, buf, len);
}
#endif
