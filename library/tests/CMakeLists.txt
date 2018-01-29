# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_executable(test_p2 test_p2.c ../src/common.c)
target_link_libraries(test_p2 streaming_algorithms)
add_test(NAME test_p2 COMMAND test_p2)

add_executable(test_running_stats test_running_stats.c ../src/common.c)
target_link_libraries(test_running_stats streaming_algorithms)
add_test(NAME test_running_stats COMMAND test_running_stats)

add_executable(test_cm_sketch test_cm_sketch.c ../src/common.c)
target_link_libraries(test_cm_sketch streaming_algorithms)
add_test(NAME test_cm_sketch COMMAND test_cm_sketch)
