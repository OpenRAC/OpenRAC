# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
#
# A game's list of differences from the original (port/game/<game>/differences.json): what the port
# does otherwise than the PS2 game, each entry a "fix" (a setting the player can switch back to the
# console's behaviour), a "guard" (keeps the game from crashing where the console carried on), an
# "always" (a fix that is always on: a soft lock, a missing feature) or a
# "port" (a PC difference, for the record). The launcher reads the same file
# (launcher/core/src/differences.rs); docs/port/SETTINGS.md section 8 says how to add an entry.
#
# openrac_write_fixes(GAME VERSION OUT [READERS files...]) checks the file and writes OUT, a C++
# table of the fixes that apply to that disc (game/common/fixes.h: key, name, default). A game
# without the file has none. A file that breaks a rule stops the configure with the reason. Each
# READERS file is host C the disc compiles: every "fix_..." key it names must be listed for the
# disc, or the fix would be always on there with no setting to switch it. A fix's "bypassed"
# ({"<disc>": "why"}) names the discs that compile the reading C but never reach the read (a disc
# that already behaves so, its C calling the game's function first): no setting there, and no stop.
#
# Included from port/CMakeLists.txt.

include_guard(GLOBAL)

set(OPENRAC_DIFFERENCES_DIR "${CMAKE_CURRENT_LIST_DIR}/../game")

# The longest name a fix may have: a row of the game's Options page holds about this many letters
# beside its value.
set(OPENRAC_FIX_NAME_MAX 22)

# `text` as the inside of a C string literal.
function(_openrac_c_string text out)
  string(REPLACE "\\" "\\\\" text "${text}")
  string(REPLACE "\"" "\\\"" text "${text}")
  set(${out} "${text}" PARENT_SCOPE)
endfunction()

function(openrac_write_fixes game version out)
  cmake_parse_arguments(PARSE_ARGV 3 arg "" "" "READERS")
  set(applied "")
  set(bypassed "")
  set(file "${OPENRAC_DIFFERENCES_DIR}/${game}/differences.json")
  set(rows "")
  set(count 0)
  if(EXISTS "${file}")
    set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${file}")
    file(READ "${file}" json)
    string(JSON file_game ERROR_VARIABLE bad GET "${json}" game)
    if(bad OR NOT file_game STREQUAL game)
      message(FATAL_ERROR "${file}: \"game\" must be \"${game}\"")
    endif()
    string(JSON n ERROR_VARIABLE bad LENGTH "${json}" differences)
    if(bad)
      message(FATAL_ERROR "${file}: no \"differences\" list (${bad})")
    endif()
    set(ids "")
    set(keys "")
    if(n GREATER 0)
      math(EXPR last "${n} - 1")
      foreach(i RANGE ${last})
        string(JSON entry GET "${json}" differences ${i})
        foreach(field id kind name original port)
          string(JSON ${field} ERROR_VARIABLE bad GET "${entry}" ${field})
          if(bad OR "${${field}}" STREQUAL "")
            message(FATAL_ERROR "${file}: entry ${i} has no \"${field}\"")
          endif()
        endforeach()
        if(NOT id MATCHES "^[a-z0-9]+(-[a-z0-9]+)*$")
          message(FATAL_ERROR "${file}: id \"${id}\" is not lower-case words joined by hyphens")
        endif()
        if(id IN_LIST ids)
          message(FATAL_ERROR "${file}: id \"${id}\" twice")
        endif()
        list(APPEND ids "${id}")
        if(NOT kind MATCHES "^(fix|guard|always|port)$")
          message(FATAL_ERROR "${file}: ${id}: kind \"${kind}\" is not fix, guard, always or port")
        endif()
        string(JSON discs_n ERROR_VARIABLE bad LENGTH "${entry}" discs)
        if(bad OR discs_n EQUAL 0)
          message(FATAL_ERROR "${file}: ${id}: no \"discs\"")
        endif()
        set(applies FALSE)
        math(EXPR discs_last "${discs_n} - 1")
        foreach(d RANGE ${discs_last})
          string(JSON disc GET "${entry}" discs ${d})
          if(NOT disc MATCHES "^(pal|ntsc|ntsc2|pal1|jp)$")
            message(FATAL_ERROR "${file}: ${id}: disc \"${disc}\" is not pal, ntsc, ntsc2, pal1 or jp")
          endif()
          if(disc STREQUAL version)
            set(applies TRUE)
          endif()
        endforeach()
        string(JSON setting ERROR_VARIABLE no_setting GET "${entry}" setting)
        string(JSON bypass ERROR_VARIABLE no_bypass GET "${entry}" bypassed)
        if(NOT kind STREQUAL "fix")
          if(NOT no_setting)
            message(FATAL_ERROR "${file}: ${id}: only a fix has a setting (a ${kind} is never switched off)")
          endif()
          if(NOT no_bypass)
            message(FATAL_ERROR "${file}: ${id}: only a fix is \"bypassed\" (its key is what host C reads)")
          endif()
          continue()
        endif()
        set(bypassed_here FALSE)
        if(NOT no_bypass)
          string(JSON bypass_n ERROR_VARIABLE bad LENGTH "${bypass}")
          if(bad OR bypass_n EQUAL 0)
            message(FATAL_ERROR "${file}: ${id}: \"bypassed\" is {\"<disc>\": \"why\"}")
          endif()
          math(EXPR bypass_last "${bypass_n} - 1")
          foreach(b RANGE ${bypass_last})
            string(JSON disc MEMBER "${bypass}" ${b})
            string(JSON why GET "${bypass}" "${disc}")
            if(NOT disc MATCHES "^(pal|ntsc|ntsc2|pal1|jp)$")
              message(FATAL_ERROR "${file}: ${id}: bypassed disc \"${disc}\" is not pal, ntsc, ntsc2, pal1 or jp")
            endif()
            if(why STREQUAL "")
              message(FATAL_ERROR "${file}: ${id}: bypassed on ${disc}: say why")
            endif()
            foreach(d RANGE ${discs_last})
              string(JSON listed GET "${entry}" discs ${d})
              if(listed STREQUAL disc)
                message(FATAL_ERROR "${file}: ${id}: ${disc} is in \"discs\" and \"bypassed\"")
              endif()
            endforeach()
            if(disc STREQUAL version)
              set(bypassed_here TRUE)
            endif()
          endforeach()
        endif()
        if(no_setting)
          message(FATAL_ERROR "${file}: ${id}: a fix needs \"setting\": {\"key\": ..., \"default\": ...}")
        endif()
        string(JSON key ERROR_VARIABLE bad GET "${setting}" key)
        if(bad OR NOT key MATCHES "^fix_[a-z0-9_]+$")
          message(FATAL_ERROR "${file}: ${id}: the setting's key must be fix_ and lower-case words")
        endif()
        if(key IN_LIST keys)
          message(FATAL_ERROR "${file}: ${id}: key ${key} twice")
        endif()
        list(APPEND keys "${key}")
        if(bypassed_here)
          list(APPEND bypassed "${key}")
        endif()
        string(JSON default ERROR_VARIABLE bad GET "${setting}" default)
        if(bad OR NOT default MATCHES "^(ON|OFF)$")
          message(FATAL_ERROR "${file}: ${id}: the setting's default must be true or false")
        endif()
        string(LENGTH "${name}" name_length)
        if(name_length GREATER OPENRAC_FIX_NAME_MAX)
          message(FATAL_ERROR "${file}: ${id}: a fix's name is a row of the game's Options page: "
                              "at most ${OPENRAC_FIX_NAME_MAX} letters")
        endif()
        if(NOT applies)
          continue()
        endif()
        _openrac_c_string("${name}" c_name)
        if(default STREQUAL "ON")
          set(on true)
        else()
          set(on false)
        endif()
        list(APPEND applied "${key}")
        string(APPEND rows "    {\"${key}\", \"${c_name}\", ${on}},\n")
        math(EXPR count "${count} + 1")
      endforeach()
    endif()
    if(count GREATER 32)
      message(FATAL_ERROR "${file}: more than 32 fixes for ${version} (fixes.h kMax)")
    endif()
  endif()
  foreach(reader IN LISTS arg_READERS)
    file(STRINGS "${reader}" lines REGEX "\"fix_[a-z0-9_]+\"")
    foreach(line IN LISTS lines)
      string(REGEX MATCHALL "\"fix_[a-z0-9_]+\"" found "${line}")
      foreach(quoted IN LISTS found)
        string(REPLACE "\"" "" key "${quoted}")
        if(NOT key IN_LIST applied AND NOT key IN_LIST bypassed)
          cmake_path(NORMAL_PATH reader OUTPUT_VARIABLE where)
          message(FATAL_ERROR "${where} reads ${key}, which port/game/${game}/differences.json does "
                              "not list for ${version}: add \"${version}\" to its \"discs\"")
        endif()
      endforeach()
    endforeach()
  endforeach()
  if(count EQUAL 0)
    set(rows "    {\"\", \"\", false},\n")
  endif()
  file(CONFIGURE OUTPUT "${out}"
    CONTENT "// Generated by port/cmake/Differences.cmake from port/game/${game}/differences.json (${version}); do not edit.
#include \"fixes.h\"

namespace openrac::fixes {

extern const Fix kTable[];
extern const int kTableCount;

const Fix kTable[] = {
${rows}};
const int kTableCount = ${count};

}  // namespace openrac::fixes
"
    @ONLY)
endfunction()
