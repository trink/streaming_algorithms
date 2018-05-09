/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua streaming algorithms time seriest binding @file */

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "lauxlib.h"
#include "lua.h"

#ifdef LUA_SANDBOX
#include <luasandbox_output.h>
#include <luasandbox_serialize.h>
#endif

#include "running_stats.h"
#include "time_series_impl.h"

static const char *g_int_mt  = "trink.streaming_algorithms.time_series_int";

static sa_time_series_int* check_ts_int(lua_State *lua, int args)
{
  sa_time_series_int *ts = luaL_checkudata(lua, 1, g_int_mt);
  luaL_argcheck(lua, args == lua_gettop(lua), 0,
                "incorrect number of arguments");
  return ts;
}


static uint64_t check_ns(lua_State *lua, int idx)
{
  double d = luaL_checknumber(lua, idx);
  luaL_argcheck(lua, d >= 0, idx, "must be postitive");
  return (uint64_t)d;
}


static int get_idx(sa_time_series_int *ts, uint64_t ns)
{
  int64_t current_row = ts->current_time / ts->ns_per_row;
  int64_t requested_row = ns / ts->ns_per_row;
  int64_t row_delta = requested_row - current_row;
  if (requested_row > current_row || abs(row_delta) >= ts->rows) {
    return -1;
  }
  return requested_row % ts->rows;
}


static int ts_new(lua_State *lua)
{
  int n = lua_gettop(lua);
  luaL_argcheck(lua, n == 2, 0, "incorrect number of arguments");
  int rows = luaL_checkint(lua, 1);
  luaL_argcheck(lua, rows > 1, 1, "must be > 1");
  double ns = luaL_checknumber(lua, 2);
  luaL_argcheck(lua, ns > 0, 2, "must be > 0");
  // the third argument will be the optional type currently just int

  sa_time_series_int *ts = lua_newuserdata(lua, sizeof(*ts) +
                                           sizeof(int) * rows);
  ts->ns_per_row = (uint64_t)ns;
  ts->rows = rows;
  sa_init_time_series_int(ts);

  luaL_getmetatable(lua, g_int_mt);
  lua_setmetatable(lua, -2);
  return 1;
}


static int ts_get_configuration_int(lua_State *lua)
{
  sa_time_series_int *ts = check_ts_int(lua, 1);
  lua_pushinteger(lua, ts->rows);
  lua_pushnumber(lua, ts->ns_per_row);
  return 2;
}


static int ts_add_int(lua_State *lua)
{
  sa_time_series_int *ts = check_ts_int(lua, 3);
  uint64_t ns = check_ns(lua, 2);
  int v = luaL_checkint(lua, 3);
  int rv = sa_add_time_series_int(ts, ns, v);
  if (rv == INT_MIN) {
    lua_pushnil(lua);
  } else {
    lua_pushinteger(lua, rv);
  }
  return 1;
}


static int ts_set_int(lua_State *lua)
{
  sa_time_series_int *ts = check_ts_int(lua, 3);
  uint64_t ns = check_ns(lua, 2);
  int v = luaL_checkint(lua, 3);
  int rv = sa_set_time_series_int(ts, ns, v);
  if (rv == INT_MIN) {
    lua_pushnil(lua);
  } else {
    lua_pushinteger(lua, rv);
  }
  return 1;
}


static int ts_merge_int(lua_State *lua)
{
  static const char *ops[] = { "add", "set", NULL };

  int i = lua_gettop(lua);
  luaL_argcheck(lua, i >= 2 && i <= 3, 0, "incorrect number of arguments");
  sa_time_series_int *ts = luaL_checkudata(lua, 1, g_int_mt);
  sa_time_series_int *ts1 = luaL_checkudata(lua, 2, g_int_mt);
  int op = luaL_checkoption(lua, 3, ops[0], ops);
  luaL_argcheck(lua, ts->ns_per_row <= ts1->ns_per_row, 3,
                "the resolution of the time series being merged must be "
                "greater than or equal to the destination");

  int (*fn)(sa_time_series_int *, uint64_t, int);
  switch (op) {
  case 0:
    fn = sa_add_time_series_int;
    break;
  case 1:
    fn = sa_set_time_series_int;
    break;
  }

  uint64_t start = ts1->current_time - ts1->ns_per_row * (ts1->rows - 1);
  int idx = ts1->current_time / ts1->ns_per_row % ts1->rows + 1;
  for (int i = 0; i < ts1->rows; ++i, ++idx) {
    if (idx == ts1->rows) {
      idx = 0;
    }
    uint64_t ns = start + i * ts1->ns_per_row;
    fn(ts, ns, ts1->v[idx]);
  }
  return 0;
}


static int ts_get_int(lua_State *lua)
{
  sa_time_series_int *ts = check_ts_int(lua, 2);
  uint64_t ns = check_ns(lua, 2);
  int rv = sa_get_time_series_int(ts, ns);
  if (rv == INT_MIN) {
    lua_pushnil(lua);
  } else {
    lua_pushinteger(lua, rv);
  }
  return 1;
}


static int ts_get_range_int(lua_State *lua)
{
  sa_time_series_int *ts = check_ts_int(lua, 3);
  uint64_t ns;
  if (lua_isnil(lua, 2)) {
    ns = ts->current_time - ts->ns_per_row * (ts->rows - 1);
  } else {
    ns = check_ns(lua, 2);
    ns = ns - (ns % ts->ns_per_row);
  }
  int n = luaL_checkint(lua, 3);
  luaL_argcheck(lua, n <= ts->rows, 3, "invalid sequence length");

  int idx = get_idx(ts, ns);
  if (idx == -1) {return 0; }

  lua_createtable(lua, n, 0);
  for (int i = 0; i < n; ++i, ++idx) {
    if (idx == ts->rows) {
      idx = 0;
    }
    lua_pushinteger(lua, (lua_Integer)ts->v[idx]);
    lua_rawseti(lua, -2, i + 1);
  }
  return 1;
}


static double stats_sum_int(sa_time_series_int *ts, int idx, int n,
                            bool include_zero, int *rows)
{
  double sum = 0;
  for (int i = 0; i < n; ++i, ++idx) {
    if (idx == ts->rows) {
      idx = 0;
    }
    int v = ts->v[idx];
    if (v != 0 || include_zero) {
      *rows += 1;
      sum += v;
    }
  }
  return sum;
}


static double stats_min_int(sa_time_series_int *ts, int idx, int n,
                            bool include_zero, int *rows)
{
  double min = INT_MAX;
  for (int i = 0; i < n; ++i, ++idx) {
    if (idx == ts->rows) {
      idx = 0;
    }
    int v = ts->v[idx];
    if (v != 0 || include_zero) {
      *rows += 1;
      if (v < min) {
        min = v;
      }
    }
  }
  return min;
}


static double stats_max_int(sa_time_series_int *ts, int idx, int n,
                            bool include_zero, int *rows)
{
  double max = INT_MIN;
  for (int i = 0; i < n; ++i, ++idx) {
    if (idx == ts->rows) {
      idx = 0;
    }
    int v = ts->v[idx];
    if (v != 0 || include_zero) {
      *rows += 1;
      if (v > max) {
        max = v;
      }
    }
  }
  return max;
}


static double stats_avg_int(sa_time_series_int *ts, int idx, int n,
                            bool include_zero, int *rows)
{
  double sum = 0;
  for (int i = 0; i < n; ++i, ++idx) {
    if (idx == ts->rows) {
      idx = 0;
    }
    int v = ts->v[idx];
    if (v != 0 || include_zero) {
      *rows += 1;
      sum += v;
    }
  }
  return *rows != 0 ? sum / *rows : 0;
}


static double stats_sd_int(sa_time_series_int *ts, int idx, int n,
                           bool include_zero, bool corrected, int *rows)
{
  sa_running_stats sd;
  sa_init_running_stats(&sd);

  for (int i = 0; i < n; ++i, ++idx) {
    if (idx == ts->rows) {
      idx = 0;
    }
    int v = ts->v[idx];
    if (v != 0 || include_zero) {
      *rows += 1;
      sa_add_running_stats(&sd, v);
    }
  }
  return corrected ? sa_sd_running_stats(&sd) : sa_usd_running_stats(&sd);
}


static int ts_stats_int(lua_State *lua)
{
  static const char *types[] = { "sum", "min", "max", "avg", "sd", "usd",
    NULL };

  int i = lua_gettop(lua);
  luaL_argcheck(lua, i >= 3 && i <= 5, 0, "incorrect number of arguments");
  sa_time_series_int *ts = luaL_checkudata(lua, 1, g_int_mt);

  uint64_t ns;
  if (lua_isnil(lua, 2)) {
    ns = ts->current_time - ts->ns_per_row * (ts->rows - 1);
  } else {
    ns = check_ns(lua, 2);
    ns = ns - (ns % ts->ns_per_row);
  }
  int n = luaL_checkint(lua, 3);
  luaL_argcheck(lua, n <= ts->rows, 3, "invalid sequence length");

  int type = luaL_checkoption(lua, 4, types[0], types);

  int include_zero = lua_toboolean(lua, 5);

  int idx = get_idx(ts, ns);
  if (idx == -1) {return 0;}

  lua_createtable(lua, n, 0);
  double result = 0;
  int rows = 0;
  switch (type) {
  case 0:
    result = stats_sum_int(ts, idx, n, include_zero, &rows);
    break;
  case 1:
    result = stats_min_int(ts, idx, n, include_zero, &rows);
    break;
  case 2:
    result = stats_max_int(ts, idx, n, include_zero, &rows);
    break;
  case 3:
    result = stats_avg_int(ts, idx, n, include_zero, &rows);
    break;
  case 4:
    result = stats_sd_int(ts, idx, n, include_zero, true, &rows);
    break;
  case 5:
    result = stats_sd_int(ts, idx, n, include_zero, false, &rows);
    break;
  }
  lua_pushnumber(lua, result);
  lua_pushinteger(lua, rows);
  return 2;
}


static int ts_mp_int(lua_State *lua)
{
  static const char *results[] = { "anomaly", "mp", "mpi", NULL };

  sa_time_series_int *ts = luaL_checkudata(lua, 1, g_int_mt);
  uint64_t ns;
  if (lua_isnil(lua, 2)) {
    ns = ts->current_time - ts->ns_per_row * (ts->rows - 1);
  } else {
    ns = check_ns(lua, 2);
    ns = ns - (ns % ts->ns_per_row);
  }
  int n = luaL_checkint(lua, 3);
  int m = luaL_checkint(lua, 4);
  luaL_argcheck(lua, n <= ts->rows && n >= 4 * m, 3,
                "invalid sequence length");
  luaL_argcheck(lua, m > 3 && n % m == 0, 4, "invalid sub-sequence length");
  double percent = luaL_checknumber(lua, 5);
  luaL_argcheck(lua, percent > 0 && percent <= 100, 5, "invalid percent");
  int result = luaL_checkoption(lua, 6, results[0], results);

  double *mp = NULL;
  int *mpi = NULL;
  int mp_len = sa_mp_time_series_int(ts, ns, n, m, percent, &mp, &mpi);
  if (mp_len == 0) {
    return 0;
  }
  int rv = 4;
  switch (result) {
  case 0:
    {
      double discord = 0;
      double idx = 0;
      sa_running_stats dis;
      sa_init_running_stats(&dis);
      for (int i = 0; i < mp_len; ++i) {
        sa_add_running_stats(&dis, mp[i]);
        if (mp[i] > discord) {
          discord = mp[i];
          idx = i;
        }
      }
      lua_pushnumber(lua, ns + idx * ts->ns_per_row);
      lua_pushnumber(lua, dis.mean);
      lua_pushnumber(lua, sa_sd_running_stats(&dis));
      lua_pushnumber(lua, discord);
    }
    break;
  case 1:
    lua_createtable(lua, mp_len, 0);
    for (int i = 0; i < mp_len; ++i) {
      lua_pushnumber(lua, mp[i]);
      lua_rawseti(lua, -2, i + 1);
    }
    rv = 1;
    break;
  case 2:
    lua_createtable(lua, mp_len, 0);
    for (int i = 0; i < mp_len; ++i) {
      lua_pushinteger(lua, (lua_Integer)mpi[i]);
      lua_rawseti(lua, -2, i + 1);
    }
    rv = 1;
    break;
  }
  free(mp);
  free(mpi);
  return rv;
}


static int ts_current_time_int(lua_State *lua)
{
  sa_time_series_int *ts = check_ts_int(lua, 1);
  lua_pushnumber(lua, (lua_Number)sa_timestamp_time_series_int(ts));
  return 1;
}


#ifdef LUA_SANDBOX
static int serialize_ts_int(lua_State *lua)
{
  lsb_output_buffer *ob = lua_touserdata(lua, -1);
  const char *key = lua_touserdata(lua, -2);
  sa_time_series_int *ts = lua_touserdata(lua, -3);
  if (!(ob && key && ts)) {
    return 1;
  }
  if (lsb_outputf(ob,
                  "if %s == nil then %s = "
                  "streaming_algorithms.time_series.new(%d, %" PRIu64 ") end\n",
                  key,
                  key,
                  ts->rows,
                  ts->ns_per_row)) {
    return 1;
  }

  if (lsb_outputf(ob, "%s:fromstring(\"", key)) {
    return 1;
  }
  size_t len;
  char *buf = sa_serialize_time_series_int(ts, &len);
  if (lsb_serialize_binary(ob, buf, len)) {
    free(buf);
    return 1;
  }
  free(buf);
  if (lsb_outputs(ob, "\")\n", 3)) {
    return 1;
  }
  return 0;
}
#endif


static int ts_tostring_int(lua_State *lua)
{
  sa_time_series_int *ts = check_ts_int(lua, 1);
  size_t len;
  char *buf = sa_serialize_time_series_int(ts, &len);
  lua_pushlstring(lua, buf, len);
  free(buf);
  return 1;
}


static int ts_fromstring_int(lua_State *lua)
{
  sa_time_series_int *ts = check_ts_int(lua, 2);
  size_t len = 0;
  const char *buf = luaL_checklstring(lua, 2, &len);
  if (sa_deserialize_time_series_int(ts, buf, len) != 0) {
    luaL_error(lua, "invalid serialization");
  }
  return 0;
}


static const struct luaL_reg ts_f[] =
{
  { "new", ts_new },
  { NULL, NULL }
};


static const struct luaL_reg ts_int_m[] =
{
  { "__tostring", ts_tostring_int },
  { "add", ts_add_int },
  { "current_time", ts_current_time_int },
  { "fromstring", ts_fromstring_int },
  { "get", ts_get_int },
  { "get_configuration", ts_get_configuration_int },
  { "get_range", ts_get_range_int },
  { "matrix_profile", ts_mp_int },
  { "merge", ts_merge_int },
  { "set", ts_set_int },
  { "stats", ts_stats_int },
  { NULL, NULL }
};


int luaopen_streaming_algorithms_time_series(lua_State *lua)
{
#ifdef LUA_SANDBOX
  lua_newtable(lua);
  lsb_add_serialize_function(lua, serialize_ts_int);
  lua_replace(lua, LUA_ENVIRONINDEX);
#endif
  luaL_newmetatable(lua, g_int_mt);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, ts_int_m);
  lua_pop(lua, 1);

  luaL_register(lua, "streaming_algorithms.time_series", ts_f);

  // if necessary flag the parent table as non-data for preservation
  lua_getglobal(lua, "streaming_algorithms");
  if (lua_getmetatable(lua, -1) == 0) {
    lua_newtable(lua);
    lua_setmetatable(lua, -2);
  } else {
    lua_pop(lua, 1);
  }
  lua_pop(lua, 1);
  return 1;
}
