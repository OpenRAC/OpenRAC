# SPDX-License-Identifier: GPL-3.0-or-later
# Native enhanced-baker executable, Android packaging and launcher-flow regression tests.
# This executable uses only the viewer's CPU level loader; it never opens a
# window or creates a graphics context. Embree stays out of viewer/game targets.
if(OPENRAC_PROBE_BAKER)
  add_executable(openrac-enhanced-baker
    # Entry points and batch execution.
    enhanced/tools/baker/batch.cpp
    enhanced/tools/baker/batch_command.cpp
    enhanced/tools/baker/main.cpp
    enhanced/tools/baker/probe_device.cpp
    # Pipeline.
    enhanced/tools/baker/pipeline/bake_json.cpp
    enhanced/tools/baker/pipeline/coordinator.cpp
    enhanced/tools/baker/pipeline/coordinator_protocol.cpp
    enhanced/tools/baker/pipeline/coordinator_streams.cpp
    enhanced/tools/baker/pipeline/pipeline.cpp
    enhanced/tools/baker/pipeline/pipeline_cache.cpp
    enhanced/tools/baker/pipeline/pipeline_config.cpp
    enhanced/tools/baker/pipeline/pipeline_events.cpp
    enhanced/tools/baker/pipeline/pipeline_level.cpp
    enhanced/tools/baker/pipeline/pipeline_sources.cpp
    enhanced/tools/baker/pipeline/pipeline_system.cpp
    enhanced/tools/baker/pipeline/stage_file.cpp
    # Sources.
    enhanced/tools/baker/sources/emission_audit.cpp
    enhanced/tools/baker/sources/emission_catalog.cpp
    enhanced/tools/baker/sources/emission_prepare.cpp
    enhanced/tools/baker/sources/interior_inputs.cpp
    enhanced/tools/baker/sources/interior_prepare.cpp
    enhanced/tools/baker/sources/interior_sources.cpp
    enhanced/tools/baker/sources/interior_stage.cpp
    # Planning.
    enhanced/tools/baker/planning/automatic_plan.cpp
    enhanced/tools/baker/planning/coverage_stage.cpp
    enhanced/tools/baker/planning/final_plan.cpp
    enhanced/tools/baker/planning/floor_matching.cpp
    enhanced/tools/baker/planning/medial_report.cpp
    enhanced/tools/baker/planning/region_boxes.cpp
    enhanced/tools/baker/planning/region_coverage.cpp
    enhanced/tools/baker/planning/region_merge.cpp
    enhanced/tools/baker/planning/region_partition.cpp
    enhanced/tools/baker/planning/region_plan.cpp
    enhanced/tools/baker/planning/region_plan_report.cpp
    enhanced/tools/baker/planning/region_refine.cpp
    enhanced/tools/baker/planning/region_seeds.cpp
    # Output.
    enhanced/tools/baker/output/batch_output.cpp
    enhanced/tools/baker/output/batch_registration.cpp
    enhanced/tools/baker/output/density_stage.cpp
    enhanced/tools/baker/output/environment_stage.cpp
    enhanced/tools/baker/output/environment_water_inputs.cpp
    enhanced/tools/baker/output/probe_output.cpp
    enhanced/tools/baker/output/report.cpp
    # Water.
    enhanced/tools/water/water_bake.cpp
    enhanced/tools/water/water_fields.cpp
    enhanced/tools/water/water_input.cpp
    enhanced/tools/water/water_mesh.cpp
    enhanced/tools/water/water_output.cpp
    enhanced/tools/water/water_raster.cpp
    enhanced/tools/water/water_source.cpp
    enhanced/tools/water/water_source_image.cpp
    enhanced/tools/water/water_source_strips.cpp
    enhanced/tools/water/water_source_tiles.cpp
  )
  target_link_libraries(openrac-enhanced-baker PRIVATE openrac_probe_scene openrac_probe_bake
    openrac_interior_inference openrac_assets_world)
  if(OPENRAC_VULKAN)
    target_link_libraries(openrac-enhanced-baker PRIVATE openrac_vulkan openrac_platform openrac_sdl3)
    target_compile_definitions(openrac-enhanced-baker PRIVATE OPENRAC_VULKAN=1)
  endif()
  openrac_warnings(openrac-enhanced-baker)
  if(Python3_FOUND AND NOT ANDROID)
    add_test(NAME enhanced_water_sources COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/enhanced/tools/baker/test_water_sources.py"
      "$<TARGET_FILE:openrac-enhanced-baker>")
    # Process startup and Tracy instrumentation slow down when CI runs bake groups together.
    set_tests_properties(enhanced_water_sources PROPERTIES TIMEOUT 60)
    add_test(NAME enhanced_baker_cli COMMAND "${Python3_EXECUTABLE}"
      "${PROJECT_SOURCE_DIR}/enhanced/tools/baker/test_baker_cli.py"
      "$<TARGET_FILE:openrac-enhanced-baker>")
    set_tests_properties(enhanced_baker_cli PROPERTIES TIMEOUT 120)
    add_test(NAME enhanced_baker_pipeline COMMAND "${Python3_EXECUTABLE}"
      "${PROJECT_SOURCE_DIR}/enhanced/tools/baker/test_native_pipeline.py"
      "$<TARGET_FILE:openrac-enhanced-baker>")
    set_tests_properties(enhanced_baker_pipeline PROPERTIES TIMEOUT 120)
    if(WIN32)
      set_tests_properties(enhanced_water_sources enhanced_baker_cli enhanced_baker_pipeline PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:embree>")
    endif()
  endif()
  if(ANDROID)
    # Android runs programs only from the app's library folder: named like a library, as the
    # games' -cli programs are (cmake/Games.cmake), finding libc++_shared.so beside it.
    set_target_properties(openrac-enhanced-baker PROPERTIES OUTPUT_NAME "libopenrac-enhanced-baker.so"
                                                        BUILD_RPATH "\$ORIGIN")
  endif()
endif()
