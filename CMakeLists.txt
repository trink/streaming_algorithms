# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.3 FATAL_ERROR)
cmake_policy(SET CMP0063 NEW)

project(streaming_algorithms_packages LANGUAGES NONE)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(PACKAGE_COMMANDS "")
if(NOT CPACK_GENERATOR)
  if(WIN32)
     set(CPACK_GENERATOR ZIP)
  else()
     set(CPACK_GENERATOR TGZ)
  endif()
endif()

include_directories("${CMAKE_SOURCE_DIR}/library/include")

include(CTest)
add_subdirectory(library)
add_subdirectory(lua_binding)

add_custom_target(packages ${PACKAGE_COMMANDS})
