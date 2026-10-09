# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
#
# A game, built from its decompilation's C: tools/hostgen writes the C again
# for the port (game memory, calls by code address), the build compiles it
# with the game's library replacements, and links it into the game's program.
#
#   openrac_add_game(rac1
#     SOURCE <decompilation>      the game version's directory (games/rac1/pal)
#     CONFIG <hostgen.json>       what hostgen reads and how (port/game/rac1)
#     HOST <files...>             the port's own C for that game (library replacements)
#     PROGRAM <files...>          the program's own sources (main.cpp))
#
# The decompilation is only read: hostgen works on copies in the build tree.

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(OPENRAC_HOSTGEN "${CMAKE_CURRENT_LIST_DIR}/../tools/hostgen/hostgen.py")

function(openrac_add_game name)
  cmake_parse_arguments(GAME "" "SOURCE;CONFIG" "HOST;PROGRAM" ${ARGN})
  set(gen "${CMAKE_CURRENT_BINARY_DIR}/games/${name}/gen")
  get_filename_component(config_dir "${GAME_CONFIG}" DIRECTORY)

  if(NOT EXISTS "${GAME_SOURCE}/src")
    message(WARNING "openrac-${name} is not built: no decompilation at ${GAME_SOURCE}")
    return()
  endif()

  # What hostgen will write, known now so the build can depend on it.
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${OPENRAC_HOSTGEN}" --game "${GAME_CONFIG}"
            --source "${GAME_SOURCE}" --out "${gen}" --list
    OUTPUT_VARIABLE listing
    RESULT_VARIABLE status)
  if(NOT status EQUAL 0)
    message(FATAL_ERROR "hostgen could not list ${name}'s sources")
  endif()
  string(REPLACE "\n" ";" listing "${listing}")
  set(outputs "")
  set(sources "")
  foreach(file IN LISTS listing)
    if(file)
      list(APPEND outputs "${gen}/${file}")
      if(file MATCHES "\\.c$")
        list(APPEND sources "${gen}/${file}")
      endif()
    endif()
  endforeach()

  file(GLOB_RECURSE inputs CONFIGURE_DEPENDS
    "${GAME_SOURCE}/src/*.c" "${GAME_SOURCE}/src/*.h" "${GAME_SOURCE}/include/*.h")
  file(GLOB tool CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../tools/hostgen/*.py")
  file(GLOB tables CONFIGURE_DEPENDS "${config_dir}/*.json" "${config_dir}/*.tsv")
  add_custom_command(
    OUTPUT ${outputs}
    COMMAND "${Python3_EXECUTABLE}" "${OPENRAC_HOSTGEN}" --game "${GAME_CONFIG}"
            --source "${GAME_SOURCE}" --out "${gen}" --quiet
    DEPENDS ${inputs} ${tool} ${tables}
    COMMENT "hostgen: ${name} from ${GAME_SOURCE}"
    VERBATIM)

  # The game: generated C and the port's C for it, compiled as the console's
  # C expects (wrapping integers, signed char, no type-based aliasing).
  add_library(openrac_${name}_game STATIC ${sources} ${GAME_HOST})
  target_include_directories(openrac_${name}_game PUBLIC "${gen}" "${config_dir}" "${config_dir}/host")
  target_link_libraries(openrac_${name}_game PUBLIC openrac_runtime)
  target_compile_options(openrac_${name}_game PRIVATE
    -fno-strict-aliasing -fwrapv -fsigned-char -ffp-contract=off)
  # The decompilation's C is read as it is: its warnings are its own.
  set_source_files_properties(${sources} PROPERTIES COMPILE_OPTIONS "-w")
  if(GAME_HOST)
    set_source_files_properties(${GAME_HOST} PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra")
  endif()

  add_executable(openrac-${name} ${GAME_PROGRAM})
  target_link_libraries(openrac-${name} PRIVATE openrac_${name}_game openrac_runtime)
  openrac_warnings(openrac-${name})
endfunction()
