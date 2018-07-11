/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set m=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief matrix unit tests @file */

#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "mu_test.h"
#include "matrix.h"


static char* test_stub()
{
  return NULL;
}


static char* test_create_matrix_int()
{
  sa_matrix_int *m = sa_create_matrix_int(2, 3);
  mu_assert(m, "creation failed");
  sa_destroy_matrix_int(m);

  m = sa_create_matrix_int(0, 1);
  mu_assert(!m, "creation success");

  m = sa_create_matrix_int(1, 0);
  mu_assert(!m, "creation success");
  return NULL;
}


static char* test_matrix_int()
{
  sa_matrix_int *m = sa_create_matrix_int(2, 1);
  mu_assert(m, "creation failed");

  // check the initial state
  mu_assert_rv(0, sa_get_matrix_int(m, 0, 0));
  mu_assert_rv(0, sa_get_matrix_int(m, 1, 0));

  int v, rv;
  // update the existing buffer
  v = sa_add_matrix_int(m, 0, 0, 10);
  rv = sa_get_matrix_int(m, 0, 0);
  mu_assert(v == rv, "expected %d received %d", v, rv);
  v = sa_add_matrix_int(m, 0, 0, -3);
  rv = sa_get_matrix_int(m, 0, 0);
  mu_assert(v == rv, "expected %d received %d", v, rv);
  v = sa_set_matrix_int(m, 0, 0, 99);
  rv = sa_get_matrix_int(m, 0, 0);
  mu_assert(v == rv, "expected %d received %d", v, rv);
  v = sa_add_matrix_int(m, 1, 0, -1);
  rv = sa_get_matrix_int(m, 1, 0);
  mu_assert(v == rv, "expected %d received %d", v, rv);

  // attempt to access out of bounds
  mu_assert_rv(INT_MIN, sa_add_matrix_int(m, -1, 2, 1));
  mu_assert_rv(INT_MIN, sa_add_matrix_int(m, 1, -2, 1));
  mu_assert_rv(INT_MIN, sa_add_matrix_int(m, 1, 2, 1));
  mu_assert_rv(INT_MIN, sa_add_matrix_int(m, 2, 1, 1));

  // overflow
  mu_assert_rv(INT_MAX - 1, sa_set_matrix_int(m, 0, 0, INT_MAX - 1));
  mu_assert_rv(INT_MAX, sa_add_matrix_int(m, 0, 0, 1));
  mu_assert_rv(INT_MAX, sa_add_matrix_int(m, 0, 0, 1));

  // underflow
  mu_assert_rv(INT_MIN + 1, sa_set_matrix_int(m, 0, 0, INT_MIN + 1));
  mu_assert_rv(INT_MIN, sa_add_matrix_int(m, 0, 0, -1));
  mu_assert_rv(INT_MIN, sa_add_matrix_int(m, 0, 0, -1));

  sa_destroy_matrix_int(m);
  return NULL;
}


static char* test_serialize_matrix_int()
{
  sa_matrix_int *t1 = sa_create_matrix_int(2, 1);
  mu_assert(t1, "creation failed");
  sa_matrix_int *t2 = sa_create_matrix_int(2, 1);
  mu_assert(t2, "creation failed");

  size_t len;
  char *s1 = sa_serialize_matrix_int(t1, &len);
  mu_assert(s1, "serialize failed");
  int rv = sa_deserialize_matrix_int(t1, s1, len);
  mu_assert(rv == 0, "received %d", rv);

  rv = sa_deserialize_matrix_int(t1, s1, len - 1);
  mu_assert(rv == 1, "received %d", rv); // invalid length

  s1[0] = '\x1';
  rv = sa_deserialize_matrix_int(t1, s1, len);
  mu_assert(rv == 2, "received %d", rv); // mis-matched rows

  s1[0] = '\x2';
  s1[4] = '\x2';
  rv = sa_deserialize_matrix_int(t1, s1, len);
  mu_assert(rv == 3, "received %d", rv); // mis-matched cols
  free(s1);

  sa_set_matrix_int(t1, 0, 0, 98);
  sa_set_matrix_int(t1, 1, 0, 99);
  mu_assert_rv(98, sa_get_matrix_int(t1, 0, 0));
  mu_assert_rv(99, sa_get_matrix_int(t1, 1, 0));
  s1 = sa_serialize_matrix_int(t1, &len);
  mu_assert(s1, "serialize failed");
  mu_assert_rv(0, sa_deserialize_matrix_int(t2, s1, len));
  mu_assert_rv(98, sa_get_matrix_int(t2, 0, 0));
  mu_assert_rv(99, sa_get_matrix_int(t2, 1, 0));
  sa_init_matrix_row_int(t2, 0);
  mu_assert_rv(0, sa_get_matrix_int(t2, 0, 0));
  mu_assert_rv(99, sa_get_matrix_int(t2, 1, 0));
  free(s1);

  sa_destroy_matrix_int(t1);
  sa_destroy_matrix_int(t2);
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_create_matrix_int);
  mu_run_test(test_matrix_int);
  mu_run_test(test_serialize_matrix_int);
  return NULL;
}


int main()
{
  char *result = all_tests();
  if (result) {
    printf("%s\n", result);
  } else {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", mu_tests_run);
  return result != NULL;
}
