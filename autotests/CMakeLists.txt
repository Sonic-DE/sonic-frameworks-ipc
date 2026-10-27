# SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_executable(frametest frametest.cpp)
target_link_libraries(frametest PRIVATE Sonic::Ipc Qt6::Test)
add_test(NAME frametest COMMAND frametest)

add_executable(servertest servertest.cpp)
target_link_libraries(servertest PRIVATE Sonic::Ipc Qt6::Test Qt6::Concurrent)
add_test(NAME servertest COMMAND servertest)
