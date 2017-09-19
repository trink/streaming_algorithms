# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_C_STANDARD 99)

if(MSVC)
    # Predefined Macros: http://msdn.microsoft.com/en_us/library/b0084kay.aspx
    # Compiler options: http://msdn.microsoft.com/en_us/library/fwkeyyhe.aspx
    # set a high warning level and treat them as errors
    set(CMAKE_C_FLAGS           "/W3 /WX")
    # debug multi threaded dll runtime, complete debugging info, runtime error checking
    set(CMAKE_C_FLAGS_DEBUG     "/MDd /Zi /RTC1")
    # multi threaded dll runtime, optimize for speed, auto inlining
    set(CMAKE_C_FLAGS_RELEASE   "/MD /O2 /Ob2 /DNDEBUG")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    # Predefined Macros: clang|gcc -dM -E -x c /dev/null
    # Compiler options: http://gcc.gnu.org/onlinedocs/gcc/Invoking_GCC.html#Invoking_GCC
    set(CMAKE_C_FLAGS         "-pedantic $ENV{CFLAGS} -Wall -Wextra")
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
endif()

set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};${PROJECT_NAME};ALL;/")
set(CPACK_PACKAGE_NAME           ${PROJECT_NAME})
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

include(GNUInstallDirs)
if(WIN32)
  set(INSTALL_CMAKE_DIR cmake)
  set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_BINDIR})
else()
  set(INSTALL_CMAKE_DIR "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake")
endif()

if(CMAKE_HOST_UNIX)
  find_library(LIBM_LIBRARY m)
endif()

include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    add_definitions(-DIS_BIG_ENDIAN)
endif()

include(CTest)
include(CPack)
include(doxygen)
