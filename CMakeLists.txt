cmake_minimum_required(VERSION 3.28)
enable_language(OBJCXX)
project(ShogiCamera LANGUAGES C CXX Swift VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SHOGI_CAMERA_DISABLE_HINT OFF)
set(SHOGI_CAMERA_ENABLE_SUNFISH OFF)

list(APPEND shogi_camera_gcc_preprocessor_definitions
  $<IF:$<CONFIG:Debug>,SHOGI_CAMERA_DEBUG=1,SHOGI_CAMERA_RELEASE=1>
  $<$<BOOL:${SHOGI_CAMERA_DISABLE_HINT}>:SHOGI_CAMERA_DISABLE_HINT=1>
  $<$<BOOL:${SHOGI_CAMERA_ENABLE_SUNFISH}>:SHOGI_CAMERA_ENABLE_SUNFISH=1>
)
list(APPEND shogi_camera_swift_active_compilation_conditions
  $<IF:$<CONFIG:Debug>,SHOGI_CAMERA_DEBUG,SHOGI_CAMERA_RELEASE>
  $<$<BOOL:${SHOGI_CAMERA_DISABLE_HINT}>:SHOGI_CAMERA_DISABLE_HINT>
  $<$<BOOL:${SHOGI_CAMERA_ENABLE_SUNFISH}>:SHOGI_CAMERA_ENABLE_SUNFISH>
)

add_subdirectory(deps/sunfish3/src/core EXCLUDE_FROM_ALL)
target_include_directories(sunfish_core PRIVATE deps/sunfish3/src)
add_subdirectory(deps/sunfish3/src/searcher EXCLUDE_FROM_ALL)
target_include_directories(sunfish_searcher PRIVATE deps/sunfish3/src)
add_subdirectory(deps/sunfish3/src/logger EXCLUDE_FROM_ALL)
target_include_directories(sunfish_logger PRIVATE deps/sunfish3/src)

find_library(opencv2 opencv2 HINTS deps/opencv/opencv/build-ios)

add_library(shogi_camera STATIC
  include/shogi_camera/shogi_camera.hpp
  src/game.cpp
  src/img.cpp
  src/move.cpp
  src/piece_book.cpp
  src/position.cpp
  src/random_ai.cpp
  src/session.cpp
  src/shogi_camera.cpp
  src/statistics.cpp
  src/sunfish3_ai.cpp
  src/piece_contour.cpp
  src/csa_adapter.cpp
  src/objc/shogi_camera.mm
  src/csa_server.cpp
  test/doctest_main.cpp
  test/move.test.hpp
  test/game.test.hpp
)
target_include_directories(shogi_camera PUBLIC
  include
  deps/sunfish3/src
  deps/base64/include
  deps/hwm.task
  deps/colormap-shaders/include
  deps/opencv/opencv/build-ios/opencv2.xcframework/ios-arm64/opencv2.framework
)
target_include_directories(shogi_camera PRIVATE
  deps/doctest
)
set_target_properties(shogi_camera PROPERTIES
  XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++20"
  XCODE_ATTRIBUTE_SWIFT_OBJC_INTEROP_MODE "objcxx"
  XCODE_ATTRIBUTE_GCC_PREPROCESSOR_DEFINITIONS "$<LIST:JOIN,${shogi_camera_gcc_preprocessor_definitions}, >"
)
target_link_libraries(shogi_camera
  $<$<BOOL:${SHOGI_CAMERA_ENABLE_SUNFISH}>:sunfish_searcher>
  $<$<BOOL:${SHOGI_CAMERA_ENABLE_SUNFISH}>:sunfish_core>
  $<$<BOOL:${SHOGI_CAMERA_ENABLE_SUNFISH}>:sunfish_logger>
)

add_executable(ShogiCameraApp MACOSX_BUNDLE
  app/AppDelegate.swift
  app/OpenCV+Utility.swift
  app/Reader.swift
  app/ShogiCamera+Utility.swift
  app/MainViewController.swift
  app/CGRect+Utility.swift
  app/Analyzer.swift
  app/VideoOverlay.swift
  app/StartView.swift
  app/GameView.swift
  app/BoardLayer.swift
  app/RoundButton.swift
  app/HelpViewController.swift
  app/StableBoardLayer.swift
  app/CGSize+Utility.swift
  app/Colors.swift
  app/LaunchScreen.storyboard
  app/Assets.xcassets
  include/shogi_camera/module.modulemap
  res/voice/voice.wav
  app/Info.plist
)
set(shogi_camera_app_resources
  app/LaunchScreen.storyboard
  app/Assets.xcassets
  res/voice/voice.wav
)
set_target_properties(ShogiCameraApp PROPERTIES
  BUNDLE YES
  RESOURCE "${shogi_camera_app_resources}"
  MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.kbinani.shogi-camera"
  MACOSX_BUNDLE_BUNDLE_NAME shogi-camera
  MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION}"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION}"
  MACOSX_BUNDLE_LONG_VERSION_STRING "${CMAKE_PROJECT_VERSION}"
  MACOSX_BUNDLE_COPYRIGHT "Copyright © kbinani"
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.github.kbinani.shogi-camera"
  XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
  XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
  XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++20"
  XCODE_ATTRIBUTE_SWIFT_OBJC_INTEROP_MODE "objcxx"
  XCODE_ATTRIBUTE_INFOPLIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/app/Info.plist"
  XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
  XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
  XCODE_ATTRIBUTE_SHOGI_CAMERA_PROJECT_VERSION "${CMAKE_PROJECT_VERSION}"
  XCODE_ATTRIBUTE_GCC_PREPROCESSOR_DEFINITIONS "$<LIST:JOIN,${shogi_camera_gcc_preprocessor_definitions}, >"
  XCODE_ATTRIBUTE_SWIFT_ACTIVE_COMPILATION_CONDITIONS "$<LIST:JOIN,${shogi_camera_swift_active_compilation_conditions}, >"
)
target_link_libraries(ShogiCameraApp
  shogi_camera
  "-framework opencv2"
)
