# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# allow access to the implementation headers
include_directories("${CMAKE_SOURCE_DIR}/library/src")

set(MODULE_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/cm_sketch.c
  ${CMAKE_CURRENT_SOURCE_DIR}/matrix.c
  ${CMAKE_CURRENT_SOURCE_DIR}/p2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/running_stats.c
  ${CMAKE_CURRENT_SOURCE_DIR}/time_series.c
  ${CMAKE_CURRENT_SOURCE_DIR}/streaming_algorithms.def
)

set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "/usr/lib/x86_64-linux-gnu")
find_package(Lua51)
if(LUA_LIBRARIES)
  find_program(LUA NAMES lua lua.bat)
  add_subdirectory(lua51)
endif()

find_package(luasandbox 1.2 CONFIG)
if(LUASANDBOX_LIBRARIES)
  add_subdirectory(luasandbox)
endif()

set(PACKAGE_COMMANDS ${PACKAGE_COMMANDS} PARENT_SCOPE)
