# SPDX-License-Identifier: GPL-3.0-or-later
# CPU HDR codecs are pinned and portable; no encoder is run by the game.
include(FetchContent)
FetchContent_Declare(openrac_astc
  GIT_REPOSITORY https://github.com/ARM-software/astc-encoder.git
  GIT_TAG 3ee54d103ca66a5b1f751ad49a4b8064235e2cf9
  GIT_SUBMODULES ""
  SOURCE_SUBDIR openrac-codec-sources-only)
FetchContent_Declare(openrac_compressonator
  GIT_REPOSITORY https://github.com/GPUOpen-Tools/compressonator.git
  GIT_TAG 9da0bc2be8d095adfab04ba9262e94842b48dac2
  GIT_SUBMODULES ""
  SOURCE_SUBDIR openrac-codec-sources-only)
# Populate only the codec sources; upstream applications and their dependencies are not built.
FetchContent_MakeAvailable(openrac_astc openrac_compressonator)
set(_astc_sources averages_and_directions block_sizes color_quantize color_unquantize
  compress_symbolic compute_variance decompress_symbolic diagnostic_trace entry
  find_best_partitioning ideal_endpoints_and_weights image integer_sequence mathlib
  mathlib_softfloat partition_tables percentile_tables pick_best_endpoint_format
  quantization symbolic_physical weight_align weight_quant_xfer_tables)
set(_astc_files)
foreach(_name IN LISTS _astc_sources)
  list(APPEND _astc_files "${openrac_astc_SOURCE_DIR}/Source/astcenc_${_name}.cpp")
endforeach()
add_library(openrac_astc_codec STATIC ${_astc_files})
target_compile_definitions(openrac_astc_codec PUBLIC
  ASTCENC_SSE=0 ASTCENC_AVX=0 ASTCENC_NEON=0 ASTCENC_SVE=0 ASTCENC_POPCNT=0 ASTCENC_F16C=0)
if(NOT OPENRAC_PROBE_BAKER)
  target_compile_definitions(openrac_astc_codec PRIVATE ASTCENC_DECOMPRESS_ONLY)
endif()
target_include_directories(openrac_astc_codec SYSTEM PUBLIC "${openrac_astc_SOURCE_DIR}/Source")
add_library(openrac_astc_decode ALIAS openrac_astc_codec)
# The pinned CPU decoder drops mode 5's high blue-Y endpoint bit; preserve it like the GPU decoder.
# Generate a corrected build copy without changing downloaded dependency sources.
file(READ "${openrac_compressonator_SOURCE_DIR}/cmp_core/shaders/bc6_encode_kernel.cpp" _bc6_source)
set(_bc6_before "bc6h_format.by = header.getvalue(61, 4);               //5:    by[3:0]")
set(_bc6_after "bc6h_format.by = header.getvalue(61, 4) |              //5:    by[3:0]")
string(FIND "${_bc6_source}" "${_bc6_before}" _bc6_fix_offset)
if(_bc6_fix_offset LESS 0)
  message(FATAL_ERROR "The pinned Compressonator mode-5 decoder patch no longer matches")
endif()
string(REPLACE "${_bc6_before}" "${_bc6_after}" _bc6_source "${_bc6_source}")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/bc6_encode_kernel.cpp" CONTENT "${_bc6_source}")
add_library(openrac_bc6 STATIC "${CMAKE_CURRENT_BINARY_DIR}/generated/bc6_encode_kernel.cpp")
target_include_directories(openrac_bc6 SYSTEM PUBLIC
  "${openrac_compressonator_SOURCE_DIR}/cmp_core/shaders"
  "${openrac_compressonator_SOURCE_DIR}/cmp_core/source"
  "${openrac_compressonator_SOURCE_DIR}/applications/_libs/cmp_math")
if(OPENRAC_PROBE_BAKER)
  add_library(openrac_astc_encode ALIAS openrac_astc_codec)
endif()

# Carry codec notices with the native executable in standalone and launcher distributions.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/enhanced/lighting/astc-LICENSE.txt"
  "${CMAKE_BINARY_DIR}/licenses/astc-encoder/LICENSE.txt" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/enhanced/lighting/compressonator-LICENSE.txt"
  "${CMAKE_BINARY_DIR}/licenses/compressonator/LICENSE.txt" COPYONLY)
