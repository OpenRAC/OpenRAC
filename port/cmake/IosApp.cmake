# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors

set(OPENRAC_IOS_BUNDLE_ID "dev.openrac.launcher.ios" CACHE STRING "iOS bundle identifier")
set(OPENRAC_IOS_GAME "rac1-ntsc" CACHE STRING "Native engine included in this iOS build")
set(OPENRAC_IOS_STORE_URL "" CACHE STRING "Optional App Store page for this distribution")
set(OPENRAC_IOS_BUILD_NUMBER "1" CACHE STRING "Apple build number")
set(OPENRAC_IOS_TEAM "" CACHE STRING "Apple Developer team")
set(OPENRAC_IOS_MOLTENVK_FRAMEWORK "" CACHE PATH "iOS MoltenVK.framework, not the macOS library")
set(OPENRAC_IOS_PYTHON_XCFRAMEWORK "" CACHE PATH "Python 3.14 iOS XCFramework for local disc preparation")
set(OPENRAC_IOS_LAUNCHER_LIBRARY "" CACHE FILEPATH "iOS launcher Rust adapter, built for aarch64-apple-ios")

option(OPENRAC_IOS_STORE_ARTWORK "Use original OpenRAC catalogue illustrations in Store bundles" OFF)

function(openrac_ios_app target)
  set(artwork_args)
  if(OPENRAC_IOS_STORE_ARTWORK)
    list(APPEND artwork_args --store-artwork)
  endif()
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/ios/bundle.py" "${PROJECT_SOURCE_DIR}/ios/engines.json" "${PROJECT_SOURCE_DIR}/../launcher/package.json")
  execute_process(COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/ios/bundle.py"
    --root "${PROJECT_SOURCE_DIR}/.." --target "${OPENRAC_IOS_GAME}"
    --store-url "${OPENRAC_IOS_STORE_URL}" ${artwork_args} --output "${CMAKE_BINARY_DIR}/ios-bundle.json"
    COMMAND_ERROR_IS_FATAL ANY)
  file(READ "${CMAKE_BINARY_DIR}/ios-bundle.json" bundle_config)
  string(JSON ios_version GET "${bundle_config}" version)
  target_compile_definitions(${target} PRIVATE "OPENRAC_IOS_GAME=\"${OPENRAC_IOS_GAME}\"")
  if(NOT EXISTS "${OPENRAC_IOS_MOLTENVK_FRAMEWORK}/MoltenVK")
    message(FATAL_ERROR "Set OPENRAC_IOS_MOLTENVK_FRAMEWORK to the iOS MoltenVK.framework")
  endif()
  set(python_framework "${OPENRAC_IOS_PYTHON_XCFRAMEWORK}/ios-arm64/Python.framework")
  if(NOT EXISTS "${python_framework}/Python")
    message(FATAL_ERROR "Set OPENRAC_IOS_PYTHON_XCFRAMEWORK to the Python 3.14 iOS XCFramework")
  endif()
  if(NOT EXISTS "${OPENRAC_IOS_LAUNCHER_LIBRARY}" OR NOT EXISTS "${PROJECT_SOURCE_DIR}/../launcher/dist/index.html")
    message(FATAL_ERROR "Build the shared frontend and openrac-launcher-ios static library first")
  endif()
  target_sources(${target} PRIVATE "${PROJECT_SOURCE_DIR}/ios/bootstrap.mm" "${PROJECT_SOURCE_DIR}/ios/prepare.mm" "${PROJECT_SOURCE_DIR}/ios/launcher.mm" "${PROJECT_SOURCE_DIR}/ios/controller.mm" "${PROJECT_SOURCE_DIR}/ios/save_transfer.mm")
  set_source_files_properties("${PROJECT_SOURCE_DIR}/ios/bootstrap.mm" "${PROJECT_SOURCE_DIR}/ios/prepare.mm" "${PROJECT_SOURCE_DIR}/ios/launcher.mm" "${PROJECT_SOURCE_DIR}/ios/controller.mm" "${PROJECT_SOURCE_DIR}/ios/save_transfer.mm"
    PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/../launcher/ios/include")
  target_include_directories(${target} SYSTEM PRIVATE "${python_framework}/Headers")
  target_link_libraries(${target} PRIVATE "-framework UIKit" "-framework GameController" "-framework UniformTypeIdentifiers" "-framework WebKit"
    "-framework Security" "${python_framework}" "${OPENRAC_IOS_LAUNCHER_LIBRARY}")
  file(GLOB_RECURSE launcher_resources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/../launcher/dist/*")
  file(GLOB_RECURSE import_resources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/ios/*.py"
    "${PROJECT_SOURCE_DIR}/../editor/*.py" "${PROJECT_SOURCE_DIR}/../tools/openrac_common/*.py"
    "${PROJECT_SOURCE_DIR}/../games/*/game.json")
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_BINARY_DIR}/ios-bundle.json" ${launcher_resources} ${import_resources} "${PROJECT_SOURCE_DIR}/ios/make_assets.swift"
    "${PROJECT_SOURCE_DIR}/../launcher/src-tauri/icons/icon.png" "${PROJECT_SOURCE_DIR}/../launcher/actions.json")
  set_target_properties(${target} PROPERTIES
    MACOSX_BUNDLE TRUE
    OUTPUT_NAME OpenRAC
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "@executable_path/Frameworks"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${ios_version}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${OPENRAC_IOS_BUILD_NUMBER}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${OPENRAC_IOS_BUNDLE_ID}"
    MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/ios/Info.plist.in"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${OPENRAC_IOS_TEAM}"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${PROJECT_SOURCE_DIR}/ios/OpenRAC.entitlements"
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks")
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_DIR:${target}>/Frameworks"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${OPENRAC_IOS_MOLTENVK_FRAMEWORK}"
            "$<TARGET_BUNDLE_DIR:${target}>/Frameworks/MoltenVK.framework"
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/ios/package.py"
            "$<TARGET_BUNDLE_DIR:${target}>" "${CMAKE_BINARY_DIR}" "${OPENRAC_IOS_PYTHON_XCFRAMEWORK}"
            "${OPENRAC_IOS_MOLTENVK_FRAMEWORK}"
    VERBATIM)
endfunction()
