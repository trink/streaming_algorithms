/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief p2 unit tests @file */

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "mu_test.h"
#include "p2.h"


static char* test_stub()
{
  return NULL;
}


static char* test_create_quantile()
{
  sa_p2_quantile *p2q = sa_create_p2_quantile(0.5);
  mu_assert(p2q, "creation failed");
  sa_destroy_p2_quantile(p2q);

  p2q = sa_create_p2_quantile(1.2);
  mu_assert(!p2q, "creation success");
  return NULL;
}


static char* test_create_histogram()
{
  sa_p2_histogram *p2h = sa_create_p2_histogram(5);
  mu_assert(p2h, "creation failed");
  sa_destroy_p2_histogram(p2h);

  p2h = sa_create_p2_histogram(2);
  mu_assert(!p2h, "creation success");
  return NULL;
}


static double obs[] = { 0.02, 0.15, 0.74, 3.39, 0.83, 22.37, 10.15, 15.43,
  38.62, 15.92, 34.60, 10.28, 1.47, 0.40, 0.05, 11.39, 0.27, 0.42, 0.09, 11.37
};
static double median = 4.44063;


static char* test_calculation_quantile()
{
  sa_p2_quantile *p2q = sa_create_p2_quantile(0.5);
  mu_assert(p2q, "creation failed");
  mu_assert(isnan(sa_estimate_p2_quantile(p2q, 2)), "expected: NaN");
  mu_assert(sa_count_p2_quantile(p2q, 2) == 0, "expected 0");

  double rpq, epq = median;
  for (size_t i = 0; i < sizeof(obs) / sizeof(double); ++i) {
    rpq = sa_add_p2_quantile(p2q, obs[i]);
  }
  mu_assert(isnan(sa_estimate_p2_quantile(p2q, 5)), "expected: NaN");
  mu_assert(fabs(rpq - epq) < .00001, "received: %g expected: %g", rpq, epq);
  rpq = sa_estimate_p2_quantile(p2q, 2);
  mu_assert(fabs(rpq - epq) < .00001, "received: %g expected: %g", rpq, epq);

  rpq = sa_estimate_p2_quantile(p2q, 0);
  epq = 0.02;
  mu_assert(fabs(rpq - epq) < .00001, "received: %g expected: %g", rpq, epq);
  rpq = sa_estimate_p2_quantile(p2q, 1);
  epq = 0.493895;
  mu_assert(fabs(rpq - epq) < .00001, "received: %g expected: %g", rpq, epq);
  rpq = sa_estimate_p2_quantile(p2q, 3);
  epq = 17.2039;
  mu_assert(fabs(rpq - epq) < .00001, "received: %g expected: %g", rpq, epq);
  rpq = sa_estimate_p2_quantile(p2q, 4);
  epq = 38.62;
  mu_assert(fabs(rpq - epq) < .00001, "received: %g expected: %g", rpq, epq);

  unsigned long long ev = 0;
  unsigned long long rv = sa_count_p2_quantile(p2q, 5);
  mu_assert(rv == ev, "received: %llu expected: %llu", rv, ev);

  rv = sa_count_p2_quantile(p2q, 0);
  ev = 1;
  mu_assert(rv == ev, "received: %llu expected: %llu", rv, ev);

  rv = sa_count_p2_quantile(p2q, 1);
  ev = 6;
  mu_assert(rv == ev, "received: %llu expected: %llu", rv, ev);

  rv = sa_count_p2_quantile(p2q, 2);
  ev = 10;
  mu_assert(rv == ev, "received: %llu expected: %llu", rv, ev);

  rv = sa_count_p2_quantile(p2q, 3);
  ev = 16;
  mu_assert(rv == ev, "received: %llu expected: %llu", rv, ev);

  rv = sa_count_p2_quantile(p2q, 4);
  ev = 20;
  mu_assert(rv == ev, "received: %llu expected: %llu", rv, ev);
  sa_destroy_p2_quantile(p2q);
  return NULL;
}


static char* test_serialize_quantile()
{
  sa_p2_quantile *q1 = sa_create_p2_quantile(0.3);
  sa_p2_quantile *q2 = sa_create_p2_quantile(0.5);
  sa_p2_quantile *q3 = sa_create_p2_quantile(0.5);
  size_t len;
  char *s1 = sa_serialize_p2_quantile(q1, &len);
  mu_assert(s1, "serialize failed");
  char *s2 = sa_serialize_p2_quantile(q2, &len);
  mu_assert(s2, "serialize failed");
  int rv = sa_deserialize_p2_quantile(q1, s1, len);
  mu_assert(rv == 0, "received %d", rv);

  rv = sa_deserialize_p2_quantile(q1, s1, len - 1);
  mu_assert(rv == 1, "received %d", rv);

  s1[3] = '\x5';
  rv = sa_deserialize_p2_quantile(q1, s1, len);
  mu_assert(rv == 3, "received %d", rv);

  double rpq;
  for (size_t i = 0; i < sizeof(obs) / sizeof(double); ++i) {
    rpq = sa_add_p2_quantile(q2, obs[i]);
  }
  free(s2);
  s2 = sa_serialize_p2_quantile(q2, &len);
  rv = sa_deserialize_p2_quantile(q3, s2, len);
  mu_assert(rv == 0, "received %d", rv);
  rpq = sa_estimate_p2_quantile(q3, 2);
  mu_assert(fabs(rpq - median) < .00001, "received: %g expected: %g", rpq,
            median);
  free(s1);
  free(s2);
  sa_destroy_p2_quantile(q1);
  sa_destroy_p2_quantile(q2);
  sa_destroy_p2_quantile(q3);
  return NULL;
}


static char* test_calculation_histogram()
{
  sa_p2_histogram *p2h = sa_create_p2_histogram(4);
  mu_assert(p2h, "creation failed");
  mu_assert(isnan(sa_estimate_p2_histogram(p2h, 2)), "expected: NaN");
  mu_assert(sa_count_p2_histogram(p2h, 2) == 0, "expected 0");

  for (size_t i = 0; i < sizeof(obs) / sizeof(double); ++i) {
    sa_add_p2_histogram(p2h, obs[i]);
  }
  mu_assert(isnan(sa_estimate_p2_histogram(p2h, 5)), "expected: NaN");
  double epq = median;
  double rpq = sa_estimate_p2_histogram(p2h, 0);
  rpq = sa_estimate_p2_histogram(p2h, 2);
  mu_assert(fabs(rpq - epq) < .00001, "received: %g expected: %g", rpq, epq);

  rpq = sa_estimate_p2_histogram(p2h, 0);
  epq = 0.02;
  mu_assert(fabs(rpq - epq) < .00001, "received: %g expected: %g", rpq, epq);
  rpq = sa_estimate_p2_histogram(p2h, 1);
  epq = 0.493895;
  mu_assert(fabs(rpq - epq) < .00001, "received: %g expected: %g", rpq, epq);
  rpq = sa_estimate_p2_histogram(p2h, 3);
  epq = 17.2039;
  mu_assert(fabs(rpq - epq) < .00001, "received: %g expected: %g", rpq, epq);
  rpq = sa_estimate_p2_histogram(p2h, 4);
  epq = 38.62;
  mu_assert(fabs(rpq - epq) < .00001, "received: %g expected: %g", rpq, epq);

  unsigned long long ev = 0;
  unsigned long long rv = sa_count_p2_histogram(p2h, 5);
  mu_assert(rv == ev, "received: %llu expected: %llu", rv, ev);

  rv = sa_count_p2_histogram(p2h, 0);
  ev = 1;
  mu_assert(rv == ev, "received: %llu expected: %llu", rv, ev);

  rv = sa_count_p2_histogram(p2h, 1);
  ev = 6;
  mu_assert(rv == ev, "received: %llu expected: %llu", rv, ev);

  rv = sa_count_p2_histogram(p2h, 2);
  ev = 10;
  mu_assert(rv == ev, "received: %llu expected: %llu", rv, ev);

  rv = sa_count_p2_histogram(p2h, 3);
  ev = 16;
  mu_assert(rv == ev, "received: %llu expected: %llu", rv, ev);

  rv = sa_count_p2_histogram(p2h, 4);
  ev = 20;
  mu_assert(rv == ev, "received: %llu expected: %llu", rv, ev);

  sa_destroy_p2_histogram(p2h);
  return NULL;
}


static char* test_serialize_histogram()
{
  sa_p2_histogram *h1 = sa_create_p2_histogram(4);
  size_t len;
  char *s1 = sa_serialize_p2_histogram(h1, &len);
  mu_assert(s1, "serialize failed");
  int rv = sa_deserialize_p2_histogram(h1, s1, len);
  mu_assert(rv == 0, "received %d", rv);

  rv = sa_deserialize_p2_histogram(h1, s1, len - 1);
  mu_assert(rv == 1, "received %d", rv);

  for (size_t i = 0; i < sizeof(obs) / sizeof(double); ++i) {
    sa_add_p2_histogram(h1, obs[i]);
  }
  free(s1);
  s1 = sa_serialize_p2_histogram(h1, &len);

  sa_p2_histogram *h2 = sa_create_p2_histogram(4);
  rv = sa_deserialize_p2_histogram(h2, s1, len);
  mu_assert(rv == 0, "received %d", rv);
  double rpq = sa_estimate_p2_histogram(h2, 2);
  mu_assert(fabs(rpq - median) < .00001, "received: %g expected: %g", rpq,
            median);
  free(s1);
  sa_destroy_p2_histogram(h1);
  sa_destroy_p2_histogram(h2);
  return NULL;
}


static char* benchmark_add_quantile()
{
  double iter = 200000;

  sa_p2_quantile *p2q = sa_create_p2_quantile(0.5);
  mu_assert(p2q, "creation failed");

  clock_t t = clock();
  for (double x = 0; x < iter; ++x) {
    sa_add_p2_quantile(p2q, x);
  }
  t = clock() - t;
  sa_destroy_p2_quantile(p2q);
  printf("benchmark quantile: %g\n", ((double)t) / CLOCKS_PER_SEC
         / iter);
  return NULL;
}


static char* benchmark_add_histogram()
{
  double iter = 200000;

  sa_p2_histogram *p2h = sa_create_p2_histogram(10);
  mu_assert(p2h, "creation failed");

  clock_t t = clock();
  for (double x = 0; x < iter; ++x) {
    sa_add_p2_histogram(p2h, x);
  }
  t = clock() - t;
  sa_destroy_p2_histogram(p2h);
  printf("benchmark histogram: %g\n", ((double)t) / CLOCKS_PER_SEC
         / iter);
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_create_quantile);
  mu_run_test(test_create_histogram);
  mu_run_test(test_calculation_quantile);
  mu_run_test(test_calculation_histogram);
  mu_run_test(test_serialize_quantile);
  mu_run_test(test_serialize_histogram);

  mu_run_test(benchmark_add_quantile);
  mu_run_test(benchmark_add_histogram);
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
