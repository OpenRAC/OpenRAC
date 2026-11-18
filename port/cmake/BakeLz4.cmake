# SPDX-License-Identifier: GPL-3.0-or-later
include(FetchContent)
FetchContent_Declare(openrac_lz4
  URL https://codeload.github.com/lz4/lz4/tar.gz/refs/tags/v1.10.0
  URL_HASH SHA256=537512904744b35e232912055ccf8ec66d768639ff3abe5788d90d792ec5f48b
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_SUBDIR openrac-codec-sources-only)
FetchContent_MakeAvailable(openrac_lz4)
set(_lz4_lib "${openrac_lz4_SOURCE_DIR}/lib")
add_library(openrac_bake_lz4 STATIC
  "${_lz4_lib}/lz4.c" "${_lz4_lib}/lz4frame.c" "${_lz4_lib}/lz4hc.c" "${_lz4_lib}/xxhash.c")
target_include_directories(openrac_bake_lz4 SYSTEM PUBLIC "${_lz4_lib}")
configure_file("${_lz4_lib}/LICENSE" "${CMAKE_BINARY_DIR}/licenses/lz4/LICENSE.txt" COPYONLY)
