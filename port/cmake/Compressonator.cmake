# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
#
# Only BC6H's CPU kernel and its headers are used; keep the pinned archive's other files unextracted.
include(FetchContent)

# Keep the existing offline override for developers with their own source checkout.
if(FETCHCONTENT_SOURCE_DIR_OPENRAC_COMPRESSONATOR)
  set(openrac_compressonator_SOURCE_DIR "${FETCHCONTENT_SOURCE_DIR_OPENRAC_COMPRESSONATOR}")
else()
  set(_compressonator_commit 9da0bc2be8d095adfab04ba9262e94842b48dac2)
  FetchContent_Declare(openrac_bc6
    URL "https://github.com/GPUOpen-Tools/compressonator/archive/${_compressonator_commit}.tar.gz"
    URL_HASH SHA256=841203fe983c53d5f8aab4c4752e580f4695450ff05e13cf7286698fb02c1d29
    DOWNLOAD_NAME compressonator.tar.gz
    DOWNLOAD_NO_EXTRACT TRUE
    SOURCE_SUBDIR openrac-codec-sources-only)
  FetchContent_MakeAvailable(openrac_bc6)
  set(_compressonator_root "compressonator-${_compressonator_commit}")
  set(openrac_compressonator_SOURCE_DIR "${openrac_bc6_SOURCE_DIR}/${_compressonator_root}")
  set(_compressonator_files
    cmp_core/shaders/bc6_common_encoder.h
    cmp_core/shaders/bc6_encode_kernel.cpp
    cmp_core/shaders/bc6_encode_kernel.h
    cmp_core/shaders/bcn_common_api.h
    cmp_core/shaders/common_def.h
    cmp_core/source/cmp_core.h
    cmp_core/source/cmp_math_func.h
    cmp_core/source/cmp_math_vec4.h
    license/corelicense.txt)
  set(_compressonator_missing FALSE)
  set(_compressonator_patterns)
  foreach(_file IN LISTS _compressonator_files)
    list(APPEND _compressonator_patterns "${_compressonator_root}/${_file}")
    if(NOT EXISTS "${openrac_compressonator_SOURCE_DIR}/${_file}")
      set(_compressonator_missing TRUE)
    endif()
  endforeach()
  # Preserve source timestamps on reconfigure, but repair an incomplete restored subset.
  if(_compressonator_missing)
    file(ARCHIVE_EXTRACT
      INPUT "${openrac_bc6_SOURCE_DIR}/compressonator.tar.gz"
      DESTINATION "${openrac_bc6_SOURCE_DIR}"
      PATTERNS ${_compressonator_patterns})
  endif()
endif()
