# SPDX-License-Identifier: GPL-3.0-or-later
# Embree belongs to the offline tool only; never link it into the game.
option(OPENRAC_PROBE_BAKER "Build the offline Embree 4 probe baker" OFF)
if(OPENRAC_PROBE_BAKER)
  find_package(embree 4 CONFIG REQUIRED)
  find_package(Threads REQUIRED)
  add_library(openrac_probe_bake STATIC
    enhanced/lighting/bake.cpp enhanced/lighting/bake_workers.cpp enhanced/lighting/bake_output_queue.cpp enhanced/lighting/bake_alignment.cpp enhanced/lighting/bake_integration.cpp
    enhanced/lighting/probe_density.cpp enhanced/lighting/probe_field_sampling.cpp
    enhanced/lighting/probe_texture_encode.cpp enhanced/lighting/probe_bc6.cpp enhanced/lighting/probe_astc.cpp enhanced/lighting/probe_astc_constraints.cpp
    enhanced/lighting/bake_material.cpp enhanced/lighting/bake_scene.cpp
    enhanced/lighting/bake_transport.cpp enhanced/lighting/bake_emitters.cpp enhanced/lighting/interior_lights.cpp
    enhanced/lighting/environment_bake.cpp enhanced/lighting/environment_placement.cpp enhanced/lighting/room_links.cpp enhanced/lighting/environment_water.cpp
    enhanced/lighting/room_layout.cpp enhanced/lighting/room_floor_graph.cpp enhanced/lighting/room_distance_field.cpp enhanced/lighting/room_wall_distance.cpp enhanced/lighting/room_medial.cpp enhanced/lighting/room_grid.cpp enhanced/lighting/environment_floor_map.cpp enhanced/lighting/environment_floor_groups.cpp enhanced/lighting/environment_space_merge.cpp enhanced/lighting/environment_probe_merge.cpp
    enhanced/lighting/environment_capture_search.cpp enhanced/lighting/environment_capture_relocation.cpp
    enhanced/lighting/environment_merge_policy.cpp)
  target_link_libraries(openrac_probe_bake PUBLIC openrac_lighting PRIVATE openrac_common embree Threads::Threads openrac_astc_encode openrac_bc6)
  openrac_warnings(openrac_probe_bake)
  openrac_add_test(environment_bake SOURCES enhanced/tests/environment_bake_test.cpp LIBS openrac_probe_bake embree)
  openrac_add_test(room_layout SOURCES enhanced/tests/room_layout_test.cpp LIBS openrac_probe_bake embree)
  openrac_add_test(room_traversal SOURCES enhanced/tests/room_traversal_test.cpp LIBS openrac_probe_bake embree)
  openrac_add_test(probe_field_sampling SOURCES enhanced/tests/probe_field_sampling_test.cpp
                  LIBS openrac_probe_bake openrac_common)
  openrac_add_test(probe_density SOURCES enhanced/tests/probe_density_test.cpp LIBS openrac_probe_bake embree)
  openrac_add_test(probe_density_stage SOURCES enhanced/tests/probe_density_stage_test.cpp
    enhanced/tools/baker/output/density_stage.cpp enhanced/tools/baker/output/report.cpp
    LIBS openrac_probe_bake openrac_common embree)
  openrac_add_test(environment_merge SOURCES enhanced/tests/environment_merge_test.cpp LIBS openrac_probe_bake embree)
  openrac_add_test(environment_floor_groups SOURCES enhanced/tests/environment_floor_groups_test.cpp
                  LIBS openrac_probe_bake embree)
  add_library(openrac_interior_inference STATIC enhanced/lighting/interiors.cpp
    enhanced/lighting/interior_labels.cpp enhanced/lighting/surface_coverage.cpp enhanced/lighting/surface_coverage_queries.cpp)
  target_include_directories(openrac_interior_inference PUBLIC .)
  target_link_libraries(openrac_interior_inference PRIVATE embree openrac_common)
  openrac_warnings(openrac_interior_inference)
  openrac_add_test(interior_inference SOURCES enhanced/tests/interior_inference_test.cpp
                  LIBS openrac_interior_inference)
  openrac_add_test(interior_lights SOURCES enhanced/tests/interior_lights_test.cpp
                  LIBS openrac_probe_bake embree)
  openrac_add_test(floor_matching SOURCES enhanced/tests/floor_matching_test.cpp
    enhanced/tools/baker/planning/floor_matching.cpp LIBS openrac_lighting openrac_common)
  openrac_add_test(probe_checksum SOURCES enhanced/tests/probe_checksum_test.cpp
    enhanced/tools/baker/pipeline/bake_json.cpp enhanced/tools/baker/pipeline/coordinator_protocol.cpp
    LIBS openrac_assets_disc openrac_common)
  openrac_add_test(probe_compression SOURCES enhanced/tests/probe_compression_test.cpp LIBS openrac_probe_bake)
  set_tests_properties(probe_compression PROPERTIES TIMEOUT 120)
  openrac_add_test(probe_bake SOURCES enhanced/tests/probe_bake_test.cpp LIBS openrac_probe_bake)
  openrac_add_test(probe_alignment SOURCES enhanced/tests/probe_alignment_test.cpp
                  LIBS openrac_probe_bake embree)
  openrac_add_test(probe_visibility SOURCES enhanced/tests/probe_visibility_test.cpp LIBS openrac_probe_bake)
  openrac_add_test(probe_mis SOURCES enhanced/tests/probe_mis_test.cpp LIBS openrac_probe_bake embree)
  openrac_add_test(probe_lighting SOURCES enhanced/tests/probe_lighting_test.cpp LIBS openrac_probe_bake)
  openrac_add_test(probe_bake_parallel SOURCES enhanced/tests/probe_bake_parallel_test.cpp
                   LIBS openrac_probe_bake)
  openrac_add_test(probe_integration SOURCES enhanced/tests/probe_integration_test.cpp
                   LIBS openrac_probe_bake)
  openrac_add_test(bake_output_queue SOURCES enhanced/tests/bake_output_queue_test.cpp
                   LIBS openrac_probe_bake openrac_common)
  set_tests_properties(bake_output_queue PROPERTIES TIMEOUT 30)
  openrac_add_test(bake_workers SOURCES enhanced/tests/bake_workers_test.cpp
                   LIBS openrac_probe_bake)
  set_tests_properties(bake_workers PROPERTIES TIMEOUT 30)
  if(WIN32)
    # The official binary package keeps Embree and its TBB DLLs together.
    set_tests_properties(environment_bake environment_merge environment_floor_groups room_layout room_traversal probe_density probe_density_stage probe_field_sampling probe_bake probe_alignment probe_bake_parallel probe_integration
      probe_visibility probe_lighting probe_mis interior_inference interior_lights PROPERTIES
      ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:embree>")
  endif()
endif()
