# SPDX-License-Identifier: MIT
# The SDK's version is the mod interface's, OPENRAC_MOD_API_MAJOR.MINOR of
# include/openrac/openrac_mod.h: a request for M.x is met by any M.y with y >= x, never by another
# major.
#
# CMake reads this file for each find_package(OpenRACMod <version>) and sets PACKAGE_VERSION_* from
# what it says.
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../include/openrac/openrac_mod.h" _openrac_api
     REGEX "^#define OPENRAC_MOD_API_(MAJOR|MINOR) [0-9]+$")
string(REGEX MATCH "MAJOR ([0-9]+)" _ "${_openrac_api}")
set(_openrac_major "${CMAKE_MATCH_1}")
string(REGEX MATCH "MINOR ([0-9]+)" _ "${_openrac_api}")
set(PACKAGE_VERSION "${_openrac_major}.${CMAKE_MATCH_1}")
# Another major is a different interface: the headers' ABI changed.
if(NOT "${PACKAGE_FIND_VERSION_MAJOR}" STREQUAL "" AND NOT PACKAGE_FIND_VERSION_MAJOR EQUAL _openrac_major)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
# A newer minor than this SDK's asks for something its headers do not have.
elseif(PACKAGE_FIND_VERSION VERSION_GREATER PACKAGE_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
    set(PACKAGE_VERSION_EXACT TRUE)
  endif()
endif()
unset(_openrac_api)
unset(_openrac_major)
