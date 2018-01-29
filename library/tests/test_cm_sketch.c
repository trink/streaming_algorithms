/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief cm_sketch unit tests @file */

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "mu_test.h"
#include "cm_sketch.h"


static char* test_stub()
{
  return NULL;
}


static char* test_create_cms()
{
  sa_cm_sketch *cms = sa_create_cms(0.0001, 0.0001);
  mu_assert(cms, "creation failed");
  sa_destroy_cms(cms);

  cms = sa_create_cms(99, 0.0001);
  mu_assert(!cms, "creation success");
  return NULL;
}


static char* test_cms()
{
  int i = 5;
  sa_cm_sketch *cms = sa_create_cms(0.1, 0.1);
  mu_assert(cms, "creation failed");

  size_t icnt = sa_item_count_cms(cms);
  mu_assert(icnt == 0, "received %" PRIu64, icnt);
  size_t ucnt = sa_unique_count_cms(cms);
  mu_assert(ucnt == 0, "received %" PRIu64, ucnt);
  int cnt = sa_point_query_cms(cms, "a", 1);
  mu_assert(cnt == 0, "received %d", cnt);

  sa_update_cms(cms, "a", 1, -10);
  cnt = sa_point_query_cms(cms, "a", 1);
  mu_assert(cnt == 0, "received %d", cnt);
  icnt = sa_item_count_cms(cms);
  mu_assert(icnt == 0, "received %" PRIu64, icnt);
  ucnt = sa_unique_count_cms(cms);
  mu_assert(ucnt == 0, "received %" PRIu64, ucnt);

  sa_update_cms(cms, "c", 1, 6);
  sa_update_cms(cms, "a", 1, 1);
  sa_update_cms(cms, "b", 1, 2);
  sa_update_cms(cms, "c", 1, -3);
  sa_update_cms(cms, &i, sizeof(int), 1);

  icnt = sa_item_count_cms(cms);
  mu_assert(icnt == 7, "received %" PRIu64, icnt);
  ucnt = sa_unique_count_cms(cms);
  mu_assert(ucnt == 4, "received %" PRIu64, ucnt);

  cnt = sa_point_query_cms(cms, "a", 1);
  mu_assert(cnt == 1, "received %d", cnt);
  cnt = sa_point_query_cms(cms, "b", 1);
  mu_assert(cnt == 2, "received %d", cnt);
  cnt = sa_point_query_cms(cms, "c", 1);
  mu_assert(cnt == 3, "received %d", cnt);
  sa_update_cms(cms, "c", 1, -4);
  cnt = sa_point_query_cms(cms, "c", 1);
  mu_assert(cnt == 0, "received %d", cnt);

  icnt = sa_item_count_cms(cms);
  mu_assert(icnt == 4, "received %" PRIu64, icnt);
  ucnt = sa_unique_count_cms(cms);
  mu_assert(ucnt == 3, "received %" PRIu64, ucnt);

  sa_destroy_cms(cms);
  return NULL;
}


static char* test_serialization()
{
  sa_cm_sketch *cms = sa_create_cms(0.1, 0.1);
  mu_assert(cms, "creation failed");
  sa_update_cms(cms, "c", 1, 3);
  sa_update_cms(cms, "a", 1, 1);
  sa_update_cms(cms, "b", 1, 2);

  size_t len;
  char *buf = sa_serialize_cms(cms, &len);
  mu_assert(buf, "serialize successful");

  sa_cm_sketch *cms1 = sa_create_cms(0.1, 0.1);
  int rv  = sa_deserialize_cms(cms1, buf, len);
  free(buf);
  mu_assert(rv == 0, "received: %d", rv);

  uint64_t icnt = sa_item_count_cms(cms1);
  mu_assert(icnt == 6, "received %" PRIu64, icnt);
  uint64_t ucnt = sa_unique_count_cms(cms1);
  mu_assert(ucnt == 3, "received %" PRIu64, ucnt);

  uint32_t cnt = sa_point_query_cms(cms1, "a", 1);
  mu_assert(cnt == 1, "received %d", cnt);
  cnt = sa_point_query_cms(cms1, "b", 1);
  mu_assert(cnt == 2, "received %d", cnt);
  cnt = sa_point_query_cms(cms1, "c", 1);
  mu_assert(cnt == 3, "received %d", cnt);

  sa_destroy_cms(cms);
  sa_destroy_cms(cms1);
  return NULL;
}


static char* benchmark_update_cms()
{
  double iter = 200000;

  sa_cm_sketch *cms = sa_create_cms(1/100000.0, 0.01);
  mu_assert(cms, "creation failed");

  clock_t t = clock();
  for (double x = 0; x < iter; ++x) {
    sa_update_cms(cms, &x, sizeof(double), 1);
  }
  t = clock() - t;
  sa_destroy_cms(cms);
  printf("benchmark histogram: %g\n", ((double)t) / CLOCKS_PER_SEC
         / iter);
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_create_cms);
  mu_run_test(test_cms);
  mu_run_test(test_serialization);

  mu_run_test(benchmark_update_cms);
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
