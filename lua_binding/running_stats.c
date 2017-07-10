/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua streaming algorithms running_stats binding @file */

#include <stdlib.h>

#include "lauxlib.h"
#include "lua.h"

#ifdef LUA_SANDBOX
#include <luasandbox_output.h>
#include <luasandbox_serialize.h>
#endif

#include "common.h"
#include "running_stats.h"

static const char *g_mt  = "trink.streaming_algorithms.running_stats";

static sa_running_stats* check_rs(lua_State *lua, int args)
{
  sa_running_stats *rs = luaL_checkudata(lua, 1, g_mt);
  luaL_argcheck(lua, args == lua_gettop(lua), 0,
                "incorrect number of arguments");
  return rs;
}


static int rs_new(lua_State *lua)
{
  int n = lua_gettop(lua);
  luaL_argcheck(lua, n == 0, 0, "this function takes no arguments");

  sa_running_stats *rs = lua_newuserdata(lua, sizeof(sa_running_stats));
  sa_init_running_stats(rs);

  luaL_getmetatable(lua, g_mt);
  lua_setmetatable(lua, -2);
  return 1;
}


static int rs_tostring(lua_State *lua)
{
  sa_running_stats *rs = check_rs(lua, 1);
  size_t len;
  char *buf = sa_serialize_running_stats(rs, &len);
  lua_pushlstring(lua, buf, len);
  free(buf);
  return 1;
}


static int rs_fromstring(lua_State *lua)
{
  sa_running_stats *rs = check_rs(lua, 2);
  size_t len = 0;
  const char *buf = luaL_checklstring(lua, 2, &len);
  if (sa_deserialize_running_stats(rs, buf, len) != 0) {
    luaL_error(lua, "invalid serialization");
  }
  return 0;
}


static int rs_add(lua_State *lua)
{
  sa_running_stats *rs = check_rs(lua, 2);
  double sample = luaL_checknumber(lua, 2);
  sa_add_running_stats(rs, sample);
  lua_pushnumber(lua, rs->mean);
  return 1;
}


static int rs_avg(lua_State *lua)
{
  sa_running_stats *rs = check_rs(lua, 1);
  lua_pushnumber(lua, rs->mean);
  return 1;
}


static int rs_clear(lua_State *lua)
{
  sa_running_stats *rs = check_rs(lua, 1);
  sa_init_running_stats(rs);
  return 0;
}


static int rs_count(lua_State *lua)
{
  sa_running_stats *rs = check_rs(lua, 1);
  lua_pushnumber(lua, rs->count);
  return 1;
}


static int rs_sd(lua_State *lua)
{
  sa_running_stats *rs = check_rs(lua, 1);
  lua_pushnumber(lua, sa_sd_running_stats(rs));
  return 1;
}


static int rs_variance(lua_State *lua)
{
  sa_running_stats *rs = check_rs(lua, 1);
  lua_pushnumber(lua, sa_variance_running_stats(rs));
  return 1;
}


#ifdef LUA_SANDBOX
static int serialize_rs(lua_State *lua)
{
  lsb_output_buffer *ob = lua_touserdata(lua, -1);
  const char *key = lua_touserdata(lua, -2);
  sa_running_stats *rs = lua_touserdata(lua, -3);
  if (!(ob && key && rs)) {
    return 1;
  }
  if (lsb_outputf(ob, "if %s == nil then %s = "
                  "streaming_algorithms.running_stats.new() end\n", key, key)) {
    return 1;
  }

  if (lsb_outputf(ob, "%s:fromstring(\"", key)) {
    return 1;
  }
  size_t len;
  char *buf = sa_serialize_running_stats(rs, &len);
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


static const struct luaL_reg rs_f[] =
{
  { "new", rs_new },
  { NULL, NULL }
};


static const struct luaL_reg rs_m[] =
{
  { "__tostring", rs_tostring },
  { "add", rs_add },
  { "avg", rs_avg },
  { "clear", rs_clear },
  { "count", rs_count },
  { "fromstring", rs_fromstring },
  { "sd", rs_sd },
  { "variance", rs_variance },
  { NULL, NULL }
};


int luaopen_streaming_algorithms_running_stats(lua_State *lua)
{
#ifdef LUA_SANDBOX
  lua_newtable(lua);
  lsb_add_serialize_function(lua, serialize_rs);
  lua_replace(lua, LUA_ENVIRONINDEX);
#endif

  luaL_newmetatable(lua, g_mt);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, rs_m);
  luaL_register(lua, g_mt + 6, rs_f);

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
