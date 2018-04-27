/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua streaming algorithms time seriest binding @file */

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
  { "set", ts_set_int },
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
