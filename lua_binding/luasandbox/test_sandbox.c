/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <luasandbox/test/mu_test.h>
#include <luasandbox/test/sandbox.h>

#include "test_module.h"

char *e = NULL;


static char* test_core()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "test.lua", TEST_MODULE_PATH, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  lsb_err_value ret = lsb_init(sb, NULL);
  mu_assert(!ret, "lsb_init() received: %s %s", ret, lsb_get_error(sb));
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  return NULL;
}


static char* test_rs()
{
  const char *output_file = "rs.preserve";

  remove(output_file);
  lsb_lua_sandbox *sb = lsb_create(NULL, "test_rs_serialize.lua",
                                   TEST_MODULE_PATH, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  lsb_err_value ret = lsb_init(sb, output_file);
  mu_assert(!ret, "lsb_init() received: %s %s", ret, lsb_get_error(sb));
  lsb_add_function(sb, &lsb_test_write_output, "write_output");

  int result = lsb_test_process(sb, 0);
  mu_assert(result == 0, "lsb_test_process() received: %d %s", result,
            lsb_get_error(sb));
  result = lsb_test_report(sb, 0);
  mu_assert(result == 0, "lsb_test_report() received: %d", result);
  mu_assert(strcmp("10", lsb_test_output) == 0, "received: %s",
            lsb_test_output);
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "test_rs_serialize.lua", TEST_MODULE_PATH, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  ret = lsb_init(sb, output_file);
  mu_assert(!ret, "lsb_init() received: %s %s", ret, lsb_get_error(sb));
  lsb_add_function(sb, &lsb_test_write_output, "write_output");

  lsb_test_report(sb, 0);
  mu_assert(strcmp("10", lsb_test_output) == 0, "received: %s",
            lsb_test_output);

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  return NULL;
}


static char* test_p2()
{
  const char *output_file = "p2.preserve";

  remove(output_file);
  lsb_lua_sandbox *sb = lsb_create(NULL, "test_p2_serialize.lua",
                                   TEST_MODULE_PATH, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  lsb_err_value ret = lsb_init(sb, output_file);
  mu_assert(!ret, "lsb_init() received: %s %s", ret, lsb_get_error(sb));
  lsb_add_function(sb, &lsb_test_write_output, "write_output");

  int result = lsb_test_process(sb, 0);
  mu_assert(result == 0, "lsb_test_process() received: %d %s", result,
            lsb_get_error(sb));
  result = lsb_test_report(sb, 0);
  mu_assert(result == 0, "lsb_test_report() received: %d", result);
  mu_assert(strcmp("20 20", lsb_test_output) == 0, "received: %s",
            lsb_test_output);
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "test_p2_serialize.lua", TEST_MODULE_PATH, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  ret = lsb_init(sb, output_file);
  mu_assert(!ret, "lsb_init() received: %s %s", ret, lsb_get_error(sb));
  lsb_add_function(sb, &lsb_test_write_output, "write_output");

  lsb_test_report(sb, 0);
  mu_assert(strcmp("20 20", lsb_test_output) == 0, "received: %s",
            lsb_test_output);

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_core);
  mu_run_test(test_rs);
  mu_run_test(test_p2);
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
  free(e);

  return result != 0;
}

