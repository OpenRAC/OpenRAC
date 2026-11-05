# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors

# Use the verified baseline's compiled data, but let each test maintain its
# own cache and discover its own prewarm manifest.
foreach(test IN LISTS TESTS)
  foreach(suffix IN ITEMS ".bin" ".bin.spirv")
    set(input "${CACHE_DIR}/render_equivalence_prepare_vulkan${suffix}")
    if(EXISTS "${input}")
      file(COPY_FILE "${input}" "${CACHE_DIR}/${test}${suffix}")
    endif()
  endforeach()
endforeach()
