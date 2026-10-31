# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Check GPU test isolation and scheduling: python -m unittest port.tools.test_gpu_tests."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


@unittest.skipUnless(shutil.which('cmake') and shutil.which('ctest'), 'CMake and CTest required')
class GpuTests(unittest.TestCase):
    def test_private_caches_and_explicit_overrides(self):
        helper = Path(__file__).resolve().parents[1] / 'cmake' / 'GpuTests.cmake'
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source, build = root / 'source', root / 'build'
            source.mkdir()
            (source / 'CMakeLists.txt').write_text('''
cmake_minimum_required(VERSION 3.24)
project(GpuTests NONE)
enable_testing()
include("''' + helper.as_posix() + '''")
foreach(name plain gpu reference shard)
  add_test(NAME ${name} COMMAND "${CMAKE_COMMAND}" -E true)
endforeach()
set_tests_properties(gpu PROPERTIES ENVIRONMENT "OPENRAC_GL_API=vulkan;OTHER=value")
set_tests_properties(reference PROPERTIES ENVIRONMENT
  "OPENRAC_GL_API=vulkan;OPENRAC_VULKAN_PIPELINE_CACHE=off")
set_tests_properties(shard PROPERTIES ENVIRONMENT "OPENRAC_GL_API=vulkan")
openrac_configure_gpu_tests(shard)
get_test_property(gpu ENVIRONMENT environment)
if(NOT "OTHER=value" IN_LIST environment OR
   NOT "OPENRAC_VULKAN_PIPELINE_CACHE=${CMAKE_CURRENT_BINARY_DIR}/test-cache/vulkan/gpu.bin" IN_LIST environment)
  message(FATAL_ERROR "Missing private cache or lost environment: ${environment}")
endif()
get_test_property(reference ENVIRONMENT environment)
if(NOT environment STREQUAL "OPENRAC_GL_API=vulkan;OPENRAC_VULKAN_PIPELINE_CACHE=off")
  message(FATAL_ERROR "Explicit cache setting changed")
endif()
foreach(name gpu reference)
  get_test_property(${name} RESOURCE_LOCK lock)
  get_test_property(${name} LABELS labels)
  if(NOT lock STREQUAL "openrac_gpu" OR NOT labels STREQUAL "gpu")
    message(FATAL_ERROR "GPU scheduling missing")
  endif()
endforeach()
get_test_property(plain RESOURCE_LOCK lock)
if(lock)
  message(FATAL_ERROR "CPU test locked")
endif()
get_test_property(shard RESOURCE_LOCK lock)
get_test_property(shard DEPENDS dependencies)
get_test_property(shard ENVIRONMENT environment)
if(lock OR NOT "gpu" IN_LIST dependencies OR NOT "reference" IN_LIST dependencies
   OR "plain" IN_LIST dependencies OR "shard" IN_LIST dependencies
   OR NOT "LP_NUM_THREADS=2" IN_LIST environment)
  message(FATAL_ERROR "Parallel shard scheduling is wrong")
endif()
''', encoding='utf-8')
            configure = ['cmake', '-S', str(source), '-B', str(build)]
            if shutil.which('ninja'):
                configure += ['-G', 'Ninja']
            configured = subprocess.run(configure,
                                        capture_output=True, text=True)
            self.assertEqual(configured.returncode, 0, configured.stdout + configured.stderr)
            cache = build / 'test-cache' / 'vulkan'
            cache.mkdir(parents=True)
            stale = [cache / ('gpu.bin' + suffix) for suffix in ('', '.manifest', '.spirv')]
            for path in stale:
                path.write_bytes(b'stale')
            unrelated = cache / 'unrelated.bin'
            unrelated.write_bytes(b'keep')
            result = subprocess.run(['ctest', '--test-dir', str(build), '-C', 'Debug', '-R', '^gpu$', '-V'],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn('vulkan_test_cache_reset', result.stdout)
            self.assertTrue(all(not path.exists() for path in stale))
            self.assertEqual(unrelated.read_bytes(), b'keep')


if __name__ == '__main__':
    unittest.main()
