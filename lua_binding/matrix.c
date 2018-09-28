/* -*- Mode: C; tab_width: 8; indent_tabs_mode: nil; c_basic_offset: 2 -*- */
/* vim: set m=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua streaming algorithms motrix binding @file */

#include <float.h>
#include <math.h>
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

static const char *g_flt_env  = "flt_env";
#endif

#include "p2.h"
#include "running_stats.h"
#include "matrix_impl.h"

static const char *g_int_mt  = "trink.streaming_algorithms.matrix_int";
static const char *g_flt_mt  = "trink.streaming_algorithms.matrix_flt";

static sa_matrix_int* check_matrix_int(lua_State *lua, int args)
{
  sa_matrix_int *m = luaL_checkudata(lua, 1, g_int_mt);
  luaL_argcheck(lua, args == lua_gettop(lua), 0,
                "incorrect number of arguments");
  return m;
}


static sa_matrix_flt* check_matrix_flt(lua_State *lua, int args)
{
  sa_matrix_flt *m = luaL_checkudata(lua, 1, g_flt_mt);
  luaL_argcheck(lua, args == lua_gettop(lua), 0,
                "incorrect number of arguments");
  return m;
}


static int matrix_new(lua_State *lua)
{
  static const char *types[] = {"int", "float", NULL};
  int n = lua_gettop(lua);
  luaL_argcheck(lua, n >= 2 && n <= 3, 0, "incorrect number of arguments");
  int rows = luaL_checkint(lua, 1);
  luaL_argcheck(lua, rows > 0, 1, "must be > 0");
  int cols = luaL_checkint(lua, 2);
  luaL_argcheck(lua, cols > 0, 2, "must be > 0");
  switch (luaL_checkoption(lua, 3, types[0], types)) {
  case 0:
    {
      sa_matrix_int *m = lua_newuserdata(lua, sizeof(*m) +
                                         sizeof(int) * rows * cols);
      m->rows = rows;
      m->cols = cols;
      sa_init_matrix_int(m);
      luaL_getmetatable(lua, g_int_mt);
    }
    break;
  case 1:
    {
      sa_matrix_flt *m = lua_newuserdata(lua, sizeof(*m) +
                                         sizeof(int) * rows * cols);
      m->rows = rows;
      m->cols = cols;
      sa_init_matrix_flt(m);
  #ifdef LUA_SANDBOX
      lua_getfield(lua, LUA_ENVIRONINDEX, g_flt_env);
      if (!lua_setfenv(lua, -2)) {
        luaL_error(lua, "failed to set the float environment");
      }
  #endif
      luaL_getmetatable(lua, g_flt_mt);
    }
    break;
  }
  lua_setmetatable(lua, -2);
  return 1;
}


static int matrix_get_configuration_int(lua_State *lua)
{
  sa_matrix_int *m = check_matrix_int(lua, 1);
  lua_pushinteger(lua, m->rows);
  lua_pushinteger(lua, m->cols);
  return 2;
}


static int matrix_get_configuration_flt(lua_State *lua)
{
  sa_matrix_flt *m = check_matrix_flt(lua, 1);
  lua_pushinteger(lua, m->rows);
  lua_pushinteger(lua, m->cols);
  return 2;
}


static int matrix_add_int(lua_State *lua)
{
  sa_matrix_int *m = check_matrix_int(lua, 4);
  int row = luaL_checkint(lua, 2) - 1;
  int col = luaL_checkint(lua, 3) - 1;
  int v = luaL_checkint(lua, 4);
  int rv = sa_add_matrix_int(m, row, col, v);
  if (rv == INT_MIN) {
    lua_pushnil(lua);
  } else {
    lua_pushinteger(lua, rv);
  }
  return 1;
}


static int matrix_add_flt(lua_State *lua)
{
  sa_matrix_flt *m = check_matrix_flt(lua, 4);
  int row = luaL_checkint(lua, 2) - 1;
  int col = luaL_checkint(lua, 3) - 1;
  double v = luaL_checknumber(lua, 4);
  float rv = sa_add_matrix_flt(m, row, col, v);
  if (rv == FLT_MIN) {
    lua_pushnil(lua);
  } else {
    lua_pushnumber(lua, rv);
  }
  return 1;
}


static int matrix_clear_row_int(lua_State *lua)
{
  sa_matrix_int *m = check_matrix_int(lua, 2);
  int row = luaL_checkint(lua, 2) - 1;
  sa_init_matrix_row_int(m, row);
  return 0;
}


static int matrix_clear_row_flt(lua_State *lua)
{
  sa_matrix_flt *m = check_matrix_flt(lua, 2);
  int row = luaL_checkint(lua, 2) - 1;
  sa_init_matrix_row_flt(m, row);
  return 0;
}


static int matrix_set_int(lua_State *lua)
{
  sa_matrix_int *m = check_matrix_int(lua, 4);
  int row = luaL_checkint(lua, 2) - 1;
  int col = luaL_checkint(lua, 3) - 1;
  int v = luaL_checkint(lua, 4);
  int rv = sa_set_matrix_int(m, row, col, v);
  if (rv == INT_MIN) {
    lua_pushnil(lua);
  } else {
    lua_pushinteger(lua, rv);
  }
  return 1;
}


static int matrix_set_flt(lua_State *lua)
{
  sa_matrix_flt *m = check_matrix_flt(lua, 4);
  int row = luaL_checkint(lua, 2) - 1;
  int col = luaL_checkint(lua, 3) - 1;
  double v = luaL_checknumber(lua, 4);
  float rv = sa_set_matrix_flt(m, row, col, v);
  if (rv == FLT_MIN) {
    lua_pushnil(lua);
  } else {
    lua_pushnumber(lua, rv);
  }
  return 1;
}


static int matrix_get_int(lua_State *lua)
{
  sa_matrix_int *m = check_matrix_int(lua, 3);
  int row = luaL_checkint(lua, 2) - 1;
  int col = luaL_checkint(lua, 3) - 1;
  int rv = sa_get_matrix_int(m, row, col);
  if (rv == INT_MIN) {
    lua_pushnil(lua);
  } else {
    lua_pushinteger(lua, rv);
  }
  return 1;
}


static int matrix_get_flt(lua_State *lua)
{
  sa_matrix_flt *m = check_matrix_flt(lua, 3);
  int row = luaL_checkint(lua, 2) - 1;
  int col = luaL_checkint(lua, 3) - 1;
  float rv = sa_get_matrix_flt(m, row, col);
  if (rv == FLT_MIN) {
    lua_pushnil(lua);
  } else {
    lua_pushnumber(lua, rv);
  }
  return 1;
}


static int matrix_get_row_int(lua_State *lua)
{
  sa_matrix_int *m = check_matrix_int(lua, 2);
  int row = luaL_checkint(lua, 2) - 1;
  if (row < 0 || row >= m->rows) {
    lua_pushnil(lua);
    return 1;
  }

  int64_t idx = (int64_t)row * m->cols;
  lua_createtable(lua, m->cols, 0);
  for (int i = 0; i < m->cols; ++i) {
    lua_pushinteger(lua, (lua_Integer)m->v[idx + i]);
    lua_rawseti(lua, -2, i + 1);
  }
  return 1;
}


static int matrix_get_row_flt(lua_State *lua)
{
  sa_matrix_flt *m = check_matrix_flt(lua, 2);
  int row = luaL_checkint(lua, 2) - 1;
  if (row < 0 || row >= m->rows) {
    lua_pushnil(lua);
    return 1;
  }

  int64_t idx = (int64_t)row * m->cols;
  lua_createtable(lua, m->cols, 0);
  for (int i = 0; i < m->cols; ++i) {
    lua_pushnumber(lua, m->v[idx + i]);
    lua_rawseti(lua, -2, i + 1);
  }
  return 1;
}


#ifdef LUA_SANDBOX
static int serialize_matrix_int(lua_State *lua)
{
  lsb_output_buffer *ob = lua_touserdata(lua, -1);
  const char *key = lua_touserdata(lua, -2);
  sa_matrix_int *m = lua_touserdata(lua, -3);
  if (!(ob && key && m)) {
    return 1;
  }
  if (lsb_outputf(ob,
                  "if %s == nil then %s = "
                  "streaming_algorithms.matrix.new(%d, %d) end\n",
                  key,
                  key,
                  m->rows,
                  m->cols)) {
    return 1;
  }

  if (lsb_outputf(ob, "%s:fromstring(\"", key)) {
    return 1;
  }
  size_t len;
  char *buf = sa_serialize_matrix_int(m, &len);
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


static int serialize_matrix_flt(lua_State *lua)
{
  lsb_output_buffer *ob = lua_touserdata(lua, -1);
  const char *key = lua_touserdata(lua, -2);
  sa_matrix_flt *m = lua_touserdata(lua, -3);
  if (!(ob && key && m)) {
    return 1;
  }
  if (lsb_outputf(ob,
                  "if %s == nil then %s = "
                  "streaming_algorithms.matrix.new(%d, %d, \"float\") end\n",
                  key,
                  key,
                  m->rows,
                  m->cols)) {
    return 1;
  }

  if (lsb_outputf(ob, "%s:fromstring(\"", key)) {
    return 1;
  }
  size_t len;
  char *buf = sa_serialize_matrix_flt(m, &len);
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


static int matrix_tostring_int(lua_State *lua)
{
  sa_matrix_int *m = check_matrix_int(lua, 1);
  size_t len;
  char *buf = sa_serialize_matrix_int(m, &len);
  lua_pushlstring(lua, buf, len);
  free(buf);
  return 1;
}


static int matrix_tostring_flt(lua_State *lua)
{
  sa_matrix_flt *m = check_matrix_flt(lua, 1);
  size_t len;
  char *buf = sa_serialize_matrix_flt(m, &len);
  lua_pushlstring(lua, buf, len);
  free(buf);
  return 1;
}


static int matrix_fromstring_int(lua_State *lua)
{
  sa_matrix_int *m = check_matrix_int(lua, 2);
  size_t len = 0;
  const char *buf = luaL_checklstring(lua, 2, &len);
  if (sa_deserialize_matrix_int(m, buf, len) != 0) {
    luaL_error(lua, "invalid serialization");
  }
  return 0;
}


static int matrix_fromstring_flt(lua_State *lua)
{
  sa_matrix_flt *m = check_matrix_flt(lua, 2);
  size_t len = 0;
  const char *buf = luaL_checklstring(lua, 2, &len);
  if (sa_deserialize_matrix_flt(m, buf, len) != 0) {
    luaL_error(lua, "invalid serialization");
  }
  return 0;
}


static double pcc_int(sa_matrix_int *m, int r, int r1,
                  sa_running_stats *rs1,
                  sa_running_stats *rs2)
{
  double sd1 = sa_usd_running_stats(rs1);
  double sd2 = sa_usd_running_stats(rs2);

  if (sd1 == 0 || sd2 == 0) {return -INFINITY;}

  double d = 0;
  for (int i = 0; i < m->cols; ++i) {
    int64_t ridx = (int64_t)r * m->cols;
    int64_t r1idx = (int64_t)r1 * m->cols;
    d += (double)m->v[ridx + i] * m->v[r1idx + i];
  }
  d = (d - m->cols * rs1->mean * rs2->mean) / (m->cols * sd1 * sd2);
  return d;
}


static double pcc_flt(sa_matrix_flt *m, int r, int r1,
                  sa_running_stats *rs1,
                  sa_running_stats *rs2)
{
  double sd1 = sa_usd_running_stats(rs1);
  double sd2 = sa_usd_running_stats(rs2);

  if (sd1 == 0 || sd2 == 0) {return -INFINITY;}

  double d = 0;
  for (int i = 0; i < m->cols; ++i) {
    int64_t ridx = (int64_t)r * m->cols;
    int64_t r1idx = (int64_t)r1 * m->cols;
    d += (double)m->v[ridx + i] * m->v[r1idx + i];
  }
  d = (d - m->cols * rs1->mean * rs2->mean) / (m->cols * sd1 * sd2);
  return d;
}


static sa_running_stats getrs_int(sa_matrix_int *m, int row)
{
  sa_running_stats rs;
  sa_init_running_stats(&rs);
  for (int i = 0; i < m->cols; ++i) {
    int64_t idx = (int64_t)row * m->cols;
    sa_add_running_stats(&rs, m->v[idx + i]);
  }
  return rs;
}


static sa_running_stats getrs_flt(sa_matrix_flt *m, int row)
{
  sa_running_stats rs;
  sa_init_running_stats(&rs);
  for (int i = 0; i < m->cols; ++i) {
    int64_t idx = (int64_t)row * m->cols;
    sa_add_running_stats(&rs, m->v[idx + i]);
  }
  return rs;
}


static int matrix_pcc_int(lua_State *lua)
{
  static const char *match_opts[] = { "max", "min", NULL };
  sa_matrix_int *m = luaL_checkudata(lua, 1, g_int_mt);
  int row = luaL_checkint(lua, 2) - 1;
  if (row < 0 || row >= m->rows) {
    lua_pushnil(lua);
    return 1;
  }
  int match = luaL_checkoption(lua, 3, match_opts[0], match_opts);

  double d = -INFINITY;
  int idx = -1;
  sa_running_stats rs = getrs_int(m, row);
  switch (match) {
  case 1:
    d = INFINITY;
    for (int i = 0; i < m->rows; ++i) {
      if (i == row) {continue;}
      sa_running_stats rs1 = getrs_int(m, i);
      double tmp = pcc_int(m, row, i, &rs, &rs1);
      if (tmp < d) {
        d = tmp;
        idx = i;
      }
    }
    break;
  default:
    for (int i = 0; i < m->rows; ++i) {
      if (i == row) {continue;}
      sa_running_stats rs1 = getrs_int(m, i);
      double tmp = pcc_int(m, row, i, &rs, &rs1);
      if (tmp > d) {
        d = tmp;
        idx = i;
      }
    }
    break;
  }
  if (!isfinite(d)) {return 0;}

  lua_pushnumber(lua, d);
  lua_pushinteger(lua, idx + 1);
  return 2;
}


static int matrix_pcc_flt(lua_State *lua)
{
  static const char *match_opts[] = { "max", "min", NULL };
  sa_matrix_flt *m = luaL_checkudata(lua, 1, g_flt_mt);
  int row = luaL_checkint(lua, 2) - 1;
  if (row < 0 || row >= m->rows) {
    lua_pushnil(lua);
    return 1;
  }
  int match = luaL_checkoption(lua, 3, match_opts[0], match_opts);

  double d = -INFINITY;
  int idx = -1;
  sa_running_stats rs = getrs_flt(m, row);
  switch (match) {
  case 1:
    d = INFINITY;
    for (int i = 0; i < m->rows; ++i) {
      if (i == row) {continue;}
      sa_running_stats rs1 = getrs_flt(m, i);
      double tmp = pcc_flt(m, row, i, &rs, &rs1);
      if (tmp < d) {
        d = tmp;
        idx = i;
      }
    }
    break;
  default:
    for (int i = 0; i < m->rows; ++i) {
      if (i == row) {continue;}
      sa_running_stats rs1 = getrs_flt(m, i);
      double tmp = pcc_flt(m, row, i, &rs, &rs1);
      if (tmp > d) {
        d = tmp;
        idx = i;
      }
    }
    break;
  }
  if (!isfinite(d)) {return 0;}

  lua_pushnumber(lua, d);
  lua_pushinteger(lua, idx + 1);
  return 2;
}


static const struct luaL_reg matrix_f[] =
{
  { "new", matrix_new },
  { NULL, NULL }
};


static const struct luaL_reg matrix_int_m[] =
{
  { "__tostring", matrix_tostring_int },
  { "add", matrix_add_int },
  { "clear_row", matrix_clear_row_int },
  { "fromstring", matrix_fromstring_int },
  { "get", matrix_get_int },
  { "get_configuration", matrix_get_configuration_int },
  { "get_row", matrix_get_row_int },
  { "set", matrix_set_int },
  { "pcc", matrix_pcc_int },
  { NULL, NULL }
};


static const struct luaL_reg matrix_flt_m[] =
{
  { "__tostring", matrix_tostring_flt },
  { "add", matrix_add_flt },
  { "clear_row", matrix_clear_row_flt },
  { "fromstring", matrix_fromstring_flt },
  { "get", matrix_get_flt },
  { "get_configuration", matrix_get_configuration_flt },
  { "get_row", matrix_get_row_flt },
  { "set", matrix_set_flt },
  { "pcc", matrix_pcc_flt },
  { NULL, NULL }
};


int luaopen_streaming_algorithms_matrix(lua_State *lua)
{
#ifdef LUA_SANDBOX
  lua_newtable(lua);
  lsb_add_serialize_function(lua, serialize_matrix_int);

  lua_newtable(lua); // create a table for the float userdata environment
  lsb_add_serialize_function(lua, serialize_matrix_flt);
  lua_setfield(lua, -2, g_flt_env);

  lua_replace(lua, LUA_ENVIRONINDEX);
#endif
  luaL_newmetatable(lua, g_int_mt);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, matrix_int_m);
  lua_pop(lua, 1);

  luaL_newmetatable(lua, g_flt_mt);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, matrix_flt_m);
  lua_pop(lua, 1);

  luaL_register(lua, "streaming_algorithms.matrix", matrix_f);

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
