# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

if(APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_INSTALL_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

add_definitions(-DDIST_VERSION="${PROJECT_VERSION}")
if(PROJECT_NAME MATCHES  "(lua51|luasandbox)_(.+)")
  set(MODULE_NAME ${CMAKE_MATCH_2})
else()
  message(FATAL_ERROR "invalid lua project name prefix: " ${PROJECT_NAME})
endif()

string(REPLACE "_" "-" CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Lua streaming algorithms module")
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};${PROJECT_NAME};ALL;/")
set(CPACK_PACKAGE_VERSION_MAJOR  ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR  ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH  ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VENDOR         "Trink")
set(CPACK_PACKAGE_CONTACT        "Mike Trinkala <trink@acm.org>")
set(CPACK_OUTPUT_CONFIG_FILE     "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.cpack")
set(CPACK_STRIP_FILES            TRUE)
set(CPACK_DEBIAN_FILE_NAME       "DEB-DEFAULT")
set(CPACK_RPM_FILE_NAME          "RPM-DEFAULT")
set(CPACK_RESOURCE_FILE_LICENSE  "${CMAKE_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RPM_PACKAGE_LICENSE    "MPLv2.0")

set(PACKAGE_COMMANDS ${PACKAGE_COMMANDS} COMMAND cpack -G ${CPACK_GENERATOR} --config ${PROJECT_NAME}.cpack PARENT_SCOPE)

set(DPERMISSION DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
set(SB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../sandboxes)
if(IS_DIRECTORY ${SB_DIR})
    install(DIRECTORY ${SB_DIR}/ DESTINATION ${INSTALL_SANDBOX_PATH} ${DPERMISSION})
endif()

set(MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../modules)
if(IS_DIRECTORY ${MODULE_DIR})
    install(DIRECTORY ${MODULE_DIR}/ DESTINATION ${INSTALL_MODULE_PATH} ${DPERMISSION})
endif()

set(IOMODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../io_modules)
if(IS_DIRECTORY ${IOMODULE_DIR})
    install(DIRECTORY ${IOMODULE_DIR}/ DESTINATION ${INSTALL_IOMODULE_PATH} ${DPERMISSION})
endif()

add_custom_target(${PROJECT_NAME}_copy_tests ALL COMMAND ${CMAKE_COMMAND} -E copy_directory
${CMAKE_CURRENT_SOURCE_DIR}/../tests
${CMAKE_CURRENT_BINARY_DIR})

if(PROJECT_NAME MATCHES "^lua51")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "lua5.1")
    set(INSTALL_MODULE_PATH ${CMAKE_INSTALL_LIBDIR}/lua)
    set(INSTALL_IOMODULE_PATH ${CMAKE_INSTALL_LIBDIR}/lua)
    set(INSTALL_SANDBOX_PATH ${CMAKE_INSTALL_DATAROOTDIR}/lua)
    if(WIN32)
        string(REPLACE "\\" "\\\\" TEST_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}\\?.lua")
        string(REGEX REPLACE "\\.lua$" ".dll" TEST_MODULE_CPATH ${TEST_MODULE_PATH})
    else()
        set(TEST_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/?.lua")
        string(REGEX REPLACE "\\.lua$" ".so" TEST_MODULE_CPATH ${TEST_MODULE_PATH})
    endif()
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${LUA} test.lua)
    set_property(TEST ${PROJECT_NAME}_test PROPERTY ENVIRONMENT
    "LUA_PATH=${TEST_MODULE_PATH}" "LUA_CPATH=${TEST_MODULE_CPATH}" TZ=UTC)
else(PROJECT_NAME MATCHES "^luasandbox")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "luasandbox (>= 1.2)")
    set(LUA_LIBRARIES ${LUASANDBOX_LIBRARIES})
    set(LUA_INCLUDE_DIR ${LUASANDBOX_INCLUDE_DIR}/luasandbox)
    set(INSTALL_MODULE_PATH ${CMAKE_INSTALL_LIBDIR}/luasandbox/modules)
    set(INSTALL_IOMODULE_PATH ${CMAKE_INSTALL_LIBDIR}/luasandbox/io_modules)
    set(INSTALL_SANDBOX_PATH ${CMAKE_INSTALL_DATAROOTDIR}/luasandbox/sandboxes)
    add_definitions(-DLUA_SANDBOX)
    if(WIN32)
        string(REPLACE "\\" "\\\\" TEST_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}\\?.lua")
        string(REGEX REPLACE "\\.lua$" ".dll" TEST_MODULE_CPATH ${TEST_MODULE_PATH})
    else()
        set(TEST_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/?.lua")
        string(REGEX REPLACE "\\.lua$" ".so" TEST_MODULE_CPATH ${TEST_MODULE_PATH})
    endif()
    configure_file(${CMAKE_SOURCE_DIR}/cmake/test_module.in.h ${CMAKE_CURRENT_BINARY_DIR}/test_module.h)
    include_directories(${CMAKE_CURRENT_BINARY_DIR} ${LUASANDBOX_INCLUDE_DIR})
    add_executable(${PROJECT_NAME}_test test_sandbox.c)
    target_link_libraries(${PROJECT_NAME}_test ${LUASANDBOX_TEST_LIBRARY} ${LUASANDBOX_LIBRARIES})
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
    if(WIN32)
       string(REPLACE "/luasandbox.lib" "" LIB_PATH ${LUASANDBOX_LIBRARY})
       message(LIB_PATH ${LIB_PATH})
       set_tests_properties(${PROJECT_NAME}_test PROPERTIES ENVIRONMENT PATH=${LIB_PATH})
    endif()
endif()

include_directories(${LUA_INCLUDE_DIR})
add_library(${PROJECT_NAME} SHARED ${MODULE_SRCS})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${MODULE_NAME})
target_link_libraries(${PROJECT_NAME} streaming_algorithms)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} ${LUA_LIBRARIES})
endif()
set(EMPTY_DIR ${CMAKE_BINARY_DIR}/empty)
file(MAKE_DIRECTORY ${EMPTY_DIR})
install(DIRECTORY ${EMPTY_DIR}/ DESTINATION ${INSTALL_MODULE_PATH} ${DPERMISSION})
install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_MODULE_PATH})
include(CPack)
