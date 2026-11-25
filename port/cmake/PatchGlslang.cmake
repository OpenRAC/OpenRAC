# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors

find_package(Git REQUIRED)
set(patch "${CMAKE_CURRENT_LIST_DIR}/glslang-operand-pool.patch")
execute_process(COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${patch}"
  WORKING_DIRECTORY "${SOURCE_DIR}" RESULT_VARIABLE already_applied ERROR_QUIET)
if(NOT already_applied STREQUAL "0")
  execute_process(COMMAND "${GIT_EXECUTABLE}" apply "${patch}"
    WORKING_DIRECTORY "${SOURCE_DIR}" COMMAND_ERROR_IS_FATAL ANY)
endif()
