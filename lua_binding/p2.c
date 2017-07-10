/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua streaming algorithms P2 binding @file */

#include <stdlib.h>
#include <string.h>

#include "lauxlib.h"
#include "lua.h"

#ifdef LUA_SANDBOX
#include <luasandbox_output.h>
#include <luasandbox_serialize.h>

static const char *g_histogram_env  = "trink.histogram_env";
#endif

#include "p2_impl.h"

static const char *g_quantile_mt  = "trink.streaming_algorithms.p2.quantile";
static const char *g_histogram_mt = "trink.streaming_algorithms.p2.histogram";

static sa_p2_quantile* check_quantile(lua_State *lua, int args)
{
  sa_p2_quantile *p2q = luaL_checkudata(lua, 1, g_quantile_mt);
  luaL_argcheck(lua, args == lua_gettop(lua), 0,
                "incorrect number of arguments");
  return p2q;
}


static int quantile_new(lua_State *lua)
{
  int n = lua_gettop(lua);
  luaL_argcheck(lua, n == 1, 0, "incorrect number of arguments");
  double quantile = luaL_checknumber(lua, 1);
  luaL_argcheck(lua, 0 < quantile && quantile < 1, 1, "0 < quantile < 1");

  sa_p2_quantile *p2q = lua_newuserdata(lua, sizeof(sa_p2_quantile));
  p2q->p = (float)quantile;
  sa_init_p2_quantile(p2q);

  luaL_getmetatable(lua, g_quantile_mt);
  lua_setmetatable(lua, -2);
  return 1;
}


static int quantile_tostring(lua_State *lua)
{
  sa_p2_quantile *p2q = check_quantile(lua, 1);
  size_t len;
  char *buf = sa_serialize_p2_quantile(p2q, &len);
  lua_pushlstring(lua, buf, len);
  free(buf);
  return 1;
}


static int quantile_fromstring(lua_State *lua)
{
  sa_p2_quantile *p2q = check_quantile(lua, 2);
  size_t len = 0;
  const char *buf = luaL_checklstring(lua, 2, &len);
  if (sa_deserialize_p2_quantile(p2q, buf, len) != 0) {
    luaL_error(lua, "invalid serialization");
  }
  return 0;
}


static int quantile_add(lua_State *lua)
{
  sa_p2_quantile *p2q = check_quantile(lua, 2);
  double value = luaL_checknumber(lua, 2);
  lua_pushnumber(lua, sa_add_p2_quantile(p2q, value));
  return 1;
}


static int quantile_clear(lua_State *lua)
{
  sa_p2_quantile *p2q = check_quantile(lua, 1);
  sa_init_p2_quantile(p2q);
  return 0;
}


static int quantile_count(lua_State *lua)
{
  sa_p2_quantile *p2q = check_quantile(lua, 2);
  double marker = luaL_checknumber(lua, 2);
  luaL_argcheck(lua, marker >= 0 && marker < QUANTILE_MARKERS, 2,
                "marker out of range");
  unsigned long long cnt = sa_count_p2_quantile(p2q, (unsigned short)marker);
  lua_pushnumber(lua, (lua_Number)cnt);
  return 1;
}


static int quantile_estimate(lua_State *lua)
{
  sa_p2_quantile *p2q = check_quantile(lua, 2);
  double marker = luaL_checknumber(lua, 2);
  luaL_argcheck(lua, marker >= 0 && marker < QUANTILE_MARKERS, 2,
                "marker out of range");
  double e = sa_estimate_p2_quantile(p2q, (unsigned short)marker);
  lua_pushnumber(lua, e);
  return 1;
}


static sa_p2_histogram* check_histogram(lua_State *lua, int args)
{
  sa_p2_histogram *p2h = luaL_checkudata(lua, 1, g_histogram_mt);
  luaL_argcheck(lua, args == lua_gettop(lua), 0,
                "incorrect number of arguments");
  return p2h;
}


static int histogram_new(lua_State *lua)
{
  int n = lua_gettop(lua);
  luaL_argcheck(lua, n == 1, 0, "incorrect number of arguments");
  double b = luaL_checknumber(lua, 1);
  luaL_argcheck(lua, 4 <= b && b < USHRT_MAX, 1, "4 <= buckets < 65535");
  unsigned short buckets = (unsigned short)b;

  size_t nbytes = sizeof(sa_p2_histogram) + sizeof(double) * (buckets + 1) * 2;
  sa_p2_histogram *p2h = lua_newuserdata(lua, nbytes);
  p2h->b = (unsigned short)buckets;
  sa_init_p2_histogram(p2h);

#ifdef LUA_SANDBOX
  lua_getfield(lua, LUA_ENVIRONINDEX, g_histogram_env);
  if (!lua_setfenv(lua, -2)) {
    luaL_error(lua, "failed to set the histogram environment");
  }
#endif

  luaL_getmetatable(lua, g_histogram_mt);
  lua_setmetatable(lua, -2);
  return 1;
}


static int histogram_tostring(lua_State *lua)
{
  sa_p2_histogram *p2h = check_histogram(lua, 1);
  size_t len;
  char *buf = sa_serialize_p2_histogram(p2h, &len);
  lua_pushlstring(lua, buf, len);
  free(buf);
  return 1;
}


static int histogram_fromstring(lua_State *lua)
{
  sa_p2_histogram *p2h = check_histogram(lua, 2);
  size_t len = 0;
  const char *buf = luaL_checklstring(lua, 2, &len);
  if (sa_deserialize_p2_histogram(p2h, buf, len) != 0) {
    luaL_error(lua, "invalid serialization");
  }
  return 0;
}


static int histogram_add(lua_State *lua)
{
  sa_p2_histogram *p2h = check_histogram(lua, 2);
  double sample = luaL_checknumber(lua, 2);
  sa_add_p2_histogram(p2h, sample);
  return 0;
}


static int histogram_clear(lua_State *lua)
{
  sa_p2_histogram *p2h = check_histogram(lua, 1);
  sa_init_p2_histogram(p2h);
  return 0;
}


static int histogram_count(lua_State *lua)
{
  sa_p2_histogram *p2h = check_histogram(lua, 2);
  double marker = luaL_checknumber(lua, 2);
  luaL_argcheck(lua, marker >= 0 && marker <= p2h->b, 2,
                "marker out of range");
  unsigned long long cnt = sa_count_p2_histogram(p2h, (unsigned short)marker);
  lua_pushnumber(lua, (lua_Number)cnt);
  return 1;
}


static int histogram_estimate(lua_State *lua)
{
  sa_p2_histogram *p2h = check_histogram(lua, 2);
  double marker = luaL_checknumber(lua, 2);
  luaL_argcheck(lua, marker >= 0 && marker <= p2h->b, 2,
                "marker out of range");
  double e = sa_estimate_p2_histogram(p2h, (unsigned short)marker);
  lua_pushnumber(lua, e);
  return 1;
}


#ifdef LUA_SANDBOX
static int serialize_quantile(lua_State *lua)
{
  lsb_output_buffer *ob = lua_touserdata(lua, -1);
  const char *key = lua_touserdata(lua, -2);
  sa_p2_quantile *p2q = lua_touserdata(lua, -3);
  if (!(ob && key && p2q)) {
    return 1;
  }
  if (lsb_outputf(ob,
                  "if %s == nil then %s = streaming_algorithms.p2.quantile(%g)"
                  " end\n",
                  key,
                  key,
                  p2q->p)) {
    return 1;
  }

  if (lsb_outputf(ob, "%s:fromstring(\"", key)) {
    return 1;
  }
  size_t len;
  char *buf = sa_serialize_p2_quantile(p2q, &len);
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


static int serialize_histogram(lua_State *lua)
{
  lsb_output_buffer *ob = lua_touserdata(lua, -1);
  const char *key = lua_touserdata(lua, -2);
  sa_p2_histogram *p2h = lua_touserdata(lua, -3);
  if (!(ob && key && p2h)) {
    return 1;
  }
  if (lsb_outputf(ob,
                  "if %s == nil then %s ="
                  " streaming_algorithms.p2.histogram(%hu) end\n",
                  key,
                  key,
                  p2h->b)) {
    return 1;
  }

  if (lsb_outputf(ob, "%s:fromstring(\"", key)) {
    return 1;
  }
  size_t len;
  char *buf = sa_serialize_p2_histogram(p2h, &len);
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


static const struct luaL_reg p2_f[] =
{
  { "histogram", histogram_new },
  { "quantile", quantile_new },
  { NULL, NULL }
};


static const struct luaL_reg quantile_m[] =
{
  { "__tostring", quantile_tostring },
  { "add", quantile_add },
  { "clear", quantile_clear },
  { "count", quantile_count },
  { "estimate", quantile_estimate },
  { "fromstring", quantile_fromstring },
  { NULL, NULL }
};


static const struct luaL_reg histogram_m[] =
{
  { "__tostring", histogram_tostring },
  { "add", histogram_add },
  { "clear", histogram_clear },
  { "count", histogram_count },
  { "estimate", histogram_estimate },
  { "fromstring", histogram_fromstring },
  { NULL, NULL }
};


int luaopen_streaming_algorithms_p2(lua_State *lua)
{
#ifdef LUA_SANDBOX
  lua_newtable(lua);
  lsb_add_serialize_function(lua, serialize_quantile);

  lua_newtable(lua); // create a table for the histogram userdata environment
  lsb_add_serialize_function(lua, serialize_histogram);
  lua_setfield(lua, -2, g_histogram_env);

  lua_replace(lua, LUA_ENVIRONINDEX);
#endif
  luaL_newmetatable(lua, g_quantile_mt);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, quantile_m);
  lua_pop(lua, 1);

  luaL_newmetatable(lua, g_histogram_mt);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, histogram_m);
  lua_pop(lua, 1);

  luaL_register(lua, "streaming_algorithms.p2", p2_f);

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
