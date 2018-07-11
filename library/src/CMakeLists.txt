# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(SA_SRCS
  common.c
  cm_sketch.c
  matrix.c
  p2.c
  running_stats.c
  time_series.c
  xxhash.c
)

add_library(${PROJECT_NAME} STATIC ${SA_SRCS})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
if(LIBM_LIBRARY)
  target_link_libraries(${PROJECT_NAME} ${LIBM_LIBRARY})
endif()
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
