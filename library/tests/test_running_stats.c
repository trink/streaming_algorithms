/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief running_stats unit tests @file */

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "mu_test.h"
#include "running_stats.h"

#ifdef _MSC_VER
// To silence the +/-INFINITY warning
#pragma warning( disable : 4756 )
#pragma warning( disable : 4056 )
#endif

static char* test_stub()
{
  return NULL;
}


static char* test_init()
{
  sa_running_stats stats;
  sa_init_running_stats(&stats);
  mu_assert(stats.count == 0, "received: %g", stats.count);
  mu_assert(stats.mean  == 0, "received: %g", stats.mean);
  mu_assert(stats.sum == 0, "received: %g", stats.sum);
  return NULL;
}


static char* test_calculation()
{
  sa_running_stats stats;
  sa_init_running_stats(&stats);
  double sd = sa_sd_running_stats(&stats);
  mu_assert(sd == 0, "received: %g", sd);
  sa_add_running_stats(&stats, 1.0);
  sa_add_running_stats(&stats, 2.0);
  sa_add_running_stats(&stats, 3.0);
  mu_assert(stats.count == 3, "received: %g", stats.count);
  mu_assert(stats.mean  == 2, "received: %g", stats.mean);
  sd = sa_sd_running_stats(&stats);
  mu_assert(sd == 1.0, "received: %g", sd);
  sd = sa_usd_running_stats(&stats);
  mu_assert(fabs(sd - 0.816497) < .000001, "received: %g", sd);
  sd = sa_variance_running_stats(&stats);
  mu_assert(sd == 1.0, "received: %g", sd);
  return NULL;
}


static char* test_nan_inf()
{
  sa_running_stats stats;
  sa_init_running_stats(&stats);
  double sd = sa_sd_running_stats(&stats);
  mu_assert(sd == 0, "received: %g", sd);
  sa_add_running_stats(&stats, INFINITY);
  sa_add_running_stats(&stats, NAN);
  sa_add_running_stats(&stats, -INFINITY);
  mu_assert(stats.count == 0, "received: %g", stats.count);
  sd = sa_sd_running_stats(&stats);
  mu_assert(sd == 0, "received: %g", sd);
  sd = sa_variance_running_stats(&stats);
  mu_assert(sd == 0, "received: %g", sd);
  return NULL;
}


static char* test_serialization()
{
  sa_running_stats stats, stats1;
  sa_init_running_stats(&stats);
  double sd = sa_sd_running_stats(&stats);
  mu_assert(sd == 0, "received: %g", sd);
  sa_add_running_stats(&stats, 1.0);
  sa_add_running_stats(&stats, 2.0);
  sa_add_running_stats(&stats, 3.0);

  size_t len;
  char *buf = sa_serialize_running_stats(&stats, &len);
  mu_assert(buf, "serialize successful");
  int rv  = sa_deserialize_running_stats(&stats1, buf, len);
  free(buf);
  mu_assert(rv == 0, "received: %d", rv);

  mu_assert(stats1.count == 3, "received: %g", stats1.count);
  mu_assert(stats1.mean  == 2, "received: %g", stats1.mean);
  sd = sa_sd_running_stats(&stats1);
  mu_assert(sd == 1.0, "received: %g", sd);
  sd = sa_variance_running_stats(&stats1);
  mu_assert(sd == 1.0, "received: %g", sd);
  return NULL;
}


static char* benchmark_update()
{
  double iter = 200000;

  sa_running_stats stats;
  sa_init_running_stats(&stats);

  clock_t t = clock();
  for (double x = 0; x < iter; ++x) {
    sa_add_running_stats(&stats, x);
  }
  t = clock() - t;
  printf("benchmark update: %g\n", ((double)t) / CLOCKS_PER_SEC / iter);
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_init);
  mu_run_test(test_calculation);
  mu_run_test(test_nan_inf);
  mu_run_test(test_serialization);

  mu_run_test(benchmark_update);
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
