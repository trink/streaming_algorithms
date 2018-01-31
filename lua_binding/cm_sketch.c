/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua streaming algorithms count-min sketch binding @file */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lauxlib.h"
#include "lua.h"

#ifdef LUA_SANDBOX
#include <luasandbox_output.h>
#include <luasandbox_serialize.h>

#endif

#include "cm_sketch_impl.h"

static const char *g_mt   = "trink.streaming_algorithms.cm_sketch";

static sa_cm_sketch* check_cms(lua_State *lua, int args)
{
  sa_cm_sketch *cms = luaL_checkudata(lua, 1, g_mt);
  luaL_argcheck(lua, args == lua_gettop(lua), 0,
                "incorrect number of arguments");
  return cms;
}


static int cms_new(lua_State *lua)
{
  int n = lua_gettop(lua);
  luaL_argcheck(lua, n == 2, 0, "incorrect number of arguments");
  double epsilon = luaL_checknumber(lua, 1);
  luaL_argcheck(lua, 0 < epsilon && epsilon < 1, 1, "0 < epsilon < 1");
  double delta = luaL_checknumber(lua, 2);
  luaL_argcheck(lua, 0 < delta && delta < 1, 2, "0 < delta < 1");

  double w = ceil(g_eulers_number / epsilon);
  double d = ceil(log(1 / delta));
  double s = w * d;
  if (s > SIZE_MAX) {
    luaL_error(lua, "invalid size");
  }

  size_t nbytes = sizeof(struct sa_cm_sketch) + sizeof(uint32_t) * (size_t)s;
  sa_cm_sketch *cms = lua_newuserdata(lua, nbytes);
  cms->width = (uint32_t)w;
  cms->depth = (uint32_t)d;
  sa_init_cms(cms);

  luaL_getmetatable(lua, g_mt);
  lua_setmetatable(lua, -2);
  return 1;
}


static int cms_tostring(lua_State *lua)
{
  sa_cm_sketch *cms = check_cms(lua, 1);
  size_t len;
  char *buf = sa_serialize_cms(cms, &len);
  lua_pushlstring(lua, buf, len);
  free(buf);
  return 1;
}


static int cms_clear(lua_State *lua)
{
  sa_cm_sketch *cms = check_cms(lua, 1);
  sa_init_cms(cms);
  return 0;
}


static int cms_fromstring(lua_State *lua)
{
  sa_cm_sketch *cms = check_cms(lua, 2);
  size_t len = 0;
  const char *buf = luaL_checklstring(lua, 2, &len);
  if (sa_deserialize_cms(cms, buf, len) != 0) {
    luaL_error(lua, "invalid serialization");
  }
  return 0;
}


static int cms_item_count(lua_State *lua)
{
  sa_cm_sketch *cms = check_cms(lua, 1);
  uint64_t cnt = sa_item_count_cms(cms);
  lua_pushnumber(lua, (lua_Number)cnt);
  return 1;
}


static int cms_point_query(lua_State *lua)
{
  sa_cm_sketch *cms = check_cms(lua, 2);
  size_t len = 0;
  double val = 0;
  void* key = NULL;
  switch (lua_type(lua, 2)) {
  case LUA_TSTRING:
    key = (void*)lua_tolstring(lua, 2, &len);
    break;
  case LUA_TNUMBER:
    val = lua_tonumber(lua, 2);
    len = sizeof(double);
    key = &val;
    break;
  default:
    luaL_argerror(lua, 2, "must be a string or number");
    break;
  }
  uint32_t cnt = sa_point_query_cms(cms, key, len);
  lua_pushnumber(lua, (lua_Number)cnt);
  return 1;
}


static int cms_unique_count(lua_State *lua)
{
  sa_cm_sketch *cms = check_cms(lua, 1);
  uint64_t cnt = sa_unique_count_cms(cms);
  lua_pushnumber(lua, (lua_Number)cnt);
  return 1;
}


static int cms_update(lua_State *lua)
{
  sa_cm_sketch *cms = luaL_checkudata(lua, 1, g_mt);
  int args = lua_gettop(lua);
  luaL_argcheck(lua, args >= 2 && args <= 3 , 0,
                "incorrect number of arguments");
  size_t len = 0;
  double val = 0;
  void* key = NULL;
  switch (lua_type(lua, 2)) {
  case LUA_TSTRING:
    key = (void*)lua_tolstring(lua, 2, &len);
    break;
  case LUA_TNUMBER:
    val = lua_tonumber(lua, 2);
    len = sizeof(double);
    key = &val;
    break;
  default:
    luaL_argerror(lua, 2, "must be a string or number");
    break;
  }
  int n = luaL_optint(lua, 3, 1);
  uint32_t cnt = sa_update_cms(cms, key, len, n);
  lua_pushnumber(lua, (lua_Number)cnt);
  return 1;
}


#ifdef LUA_SANDBOX
static int serialize_cms(lua_State *lua)
{
  lsb_output_buffer *ob = lua_touserdata(lua, -1);
  const char *key = lua_touserdata(lua, -2);
  sa_cm_sketch *cms = lua_touserdata(lua, -3);
  if (!(ob && key && cms)) {
    return 1;
  }

  double epsilon = g_eulers_number / (cms->width - 0.5);
  double delta = 1.0 / pow(g_eulers_number, (cms->depth - 0.5));
  if (lsb_outputf(ob,
                  "if %s == nil then %s ="
                  " streaming_algorithms.cm_sketch(%g, %g) end\n",
                  key,
                  key,
                  epsilon, delta)) {
    return 1;
  }

  if (lsb_outputf(ob, "%s:fromstring(\"", key)) {
    return 1;
  }
  size_t len;
  char *buf = sa_serialize_cms(cms, &len);
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


static const struct luaL_reg cm_sketch_f[] =
{
  { "new", cms_new },
  { NULL, NULL }
};


static const struct luaL_reg cm_sketch_m[] =
{
  { "__tostring", cms_tostring },
  { "clear", cms_clear },
  { "fromstring", cms_fromstring },
  { "item_count", cms_item_count },
  { "point_query", cms_point_query },
  { "unique_count", cms_unique_count },
  { "update", cms_update },
  { NULL, NULL }
};


int luaopen_streaming_algorithms_cm_sketch(lua_State *lua)
{
#ifdef LUA_SANDBOX
  lua_newtable(lua);
  lsb_add_serialize_function(lua, serialize_cms);
  lua_replace(lua, LUA_ENVIRONINDEX);
#endif
  luaL_newmetatable(lua, g_mt);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, cm_sketch_m);
  lua_pop(lua, 1);

  luaL_register(lua, "streaming_algorithms.cm_sketch", cm_sketch_f);

  // if necessary flag the parent table as non-data to prevent preservation
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
