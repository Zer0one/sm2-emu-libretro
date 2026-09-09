# SPDX-License-Identifier: BSD-3-Clause
add_executable(sm2-headless main.cpp)
target_link_libraries(sm2-headless PRIVATE sm2_hw sm2_render sm2_warnings)
# GameDatabase::locate() also looks beside a plain executable on macOS.
configure_file("${CMAKE_SOURCE_DIR}/data/games.xml"
               "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/games.xml" COPYONLY)
