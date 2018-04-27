/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief time_series unit tests @file */

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>

#include "mu_test.h"
#include "time_series.h"


static char* test_stub()
{
  return NULL;
}


static char* test_create_time_series_int()
{
  sa_time_series_int *ts = sa_create_time_series_int(86400, 1000000000);
  mu_assert(ts, "creation failed");
  uint64_t v = 86399ULL * 1000000000;
  uint64_t rv = sa_timestamp_time_series_int(ts);
  mu_assert(v == rv, "expected %" PRIu64 " received %" PRIu64, v, rv);
  sa_destroy_time_series_int(ts);

  ts = sa_create_time_series_int(1, 1);
  mu_assert(!ts, "creation success");

  ts = sa_create_time_series_int(2, 0);
  mu_assert(!ts, "creation success");
  return NULL;
}


static char* test_time_series_int()
{
  sa_time_series_int *ts = sa_create_time_series_int(2, 1);
  mu_assert(ts, "creation failed");

  // check the initial state
  mu_assert_rv(0, sa_get_time_series_int(ts, 0));
  mu_assert_rv(0, sa_get_time_series_int(ts, 1));

  int v, rv;
  // update the existing buffer
  v = sa_add_time_series_int(ts, 0, 10);
  rv = sa_get_time_series_int(ts, 0);
  mu_assert(v == rv, "expected %d received %d", v, rv);
  v = sa_add_time_series_int(ts, 0, -3);
  rv = sa_get_time_series_int(ts, 0);
  mu_assert(v == rv, "expected %d received %d", v, rv);
  v = sa_set_time_series_int(ts, 0, 99);
  rv = sa_get_time_series_int(ts, 0);
  mu_assert(v == rv, "expected %d received %d", v, rv);
  v = sa_add_time_series_int(ts, 1, -1);
  rv = sa_get_time_series_int(ts, 1);
  mu_assert(v == rv, "expected %d received %d", v, rv);

  // attempt to read into the future
  mu_assert_rv(INT_MIN, sa_get_time_series_int(ts, 10));

  // advance the buffer by 1
  v = sa_add_time_series_int(ts, 2, 11);
  rv = sa_get_time_series_int(ts, 2);
  mu_assert(v == rv, "expected %d received %d", v, rv);
  mu_assert_rv(-1, sa_get_time_series_int(ts, 1));

  // advance the buffer by 2
  v = sa_add_time_series_int(ts, 4, 22);
  rv = sa_get_time_series_int(ts, 4);
  mu_assert(v == rv, "expected %d received %d", v, rv);
  mu_assert_rv(0, sa_get_time_series_int(ts, 3));

  // advance the buffer by 6
  v = sa_add_time_series_int(ts, 10, 66);
  rv = sa_get_time_series_int(ts, 10);
  mu_assert(v == rv, "expected %d received %d", v, rv);
  mu_assert_rv(0, sa_get_time_series_int(ts, 9));

  // attempt to access the past
  mu_assert_rv(INT_MIN, sa_add_time_series_int(ts, 1, -98));
  mu_assert_rv(INT_MIN, sa_set_time_series_int(ts, 1, -99));
  mu_assert_rv(INT_MIN, sa_get_time_series_int(ts, 1));

  // overflow
  mu_assert_rv(INT_MAX - 1, sa_set_time_series_int(ts, 10, INT_MAX - 1));
  mu_assert_rv(INT_MAX, sa_add_time_series_int(ts, 10, 1));
  mu_assert_rv(INT_MAX, sa_add_time_series_int(ts, 10, 1));

  // underflow
  mu_assert_rv(INT_MIN + 1, sa_set_time_series_int(ts, 10, INT_MIN + 1));
  mu_assert_rv(INT_MIN, sa_add_time_series_int(ts, 10, -1));
  mu_assert_rv(INT_MIN, sa_add_time_series_int(ts, 10, -1));

  sa_destroy_time_series_int(ts);
  return NULL;
}


static char* test_serialize_time_series_int()
{
  sa_time_series_int *t1 = sa_create_time_series_int(2, 1);
  mu_assert(t1, "creation failed");
  sa_time_series_int *t2 = sa_create_time_series_int(2, 1);
  mu_assert(t2, "creation failed");

  size_t len;
  char *s1 = sa_serialize_time_series_int(t1, &len);
  mu_assert(s1, "serialize failed");
  int rv = sa_deserialize_time_series_int(t1, s1, len);
  mu_assert(rv == 0, "received %d", rv);

  rv = sa_deserialize_time_series_int(t1, s1, len - 1);
  mu_assert(rv == 1, "received %d", rv); // invalid length

  s1[8] = '\x2';
  rv = sa_deserialize_time_series_int(t1, s1, len);
  mu_assert(rv == 2, "received %d", rv); // mis-matched ns_per_row

  s1[8] = '\x1';
  s1[16] = '\x3';
  rv = sa_deserialize_time_series_int(t1, s1, len);
  mu_assert(rv == 3, "received %d", rv); // mis-matched rows
  free(s1);

  sa_set_time_series_int(t1, 0, 98);
  sa_set_time_series_int(t1, 1, 99);
  s1 = sa_serialize_time_series_int(t1, &len);
  mu_assert(s1, "serialize failed");
  mu_assert_rv(0, sa_deserialize_time_series_int(t2, s1, len));
  mu_assert_rv(98, sa_get_time_series_int(t2, 0));
  mu_assert_rv(99, sa_get_time_series_int(t2, 1));
  free(s1);

  sa_destroy_time_series_int(t1);
  sa_destroy_time_series_int(t2);
  return NULL;
}


static char* benchmark_add_time_series_int()
{
  int iter = 1000000;

  sa_time_series_int *ts = sa_create_time_series_int(2, 1);
  mu_assert(ts, "creation failed");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    sa_add_time_series_int(ts, x, x);
  }
  t = clock() - t;
  sa_destroy_time_series_int(ts);
  printf("benchmark add_time_series: %g\n", ((double)t) / CLOCKS_PER_SEC
         / iter);
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_create_time_series_int);
  mu_run_test(test_time_series_int);
  mu_run_test(test_serialize_time_series_int);

  mu_run_test(benchmark_add_time_series_int);
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
