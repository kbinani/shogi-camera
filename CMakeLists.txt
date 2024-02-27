cmake_minimum_required(VERSION 3.28)
enable_language(OBJCXX)
project(shogi-camera LANGUAGES C CXX Swift VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(deps/sunfish3/src EXCLUDE_FROM_ALL)

add_library(shogi-camera STATIC
  include/shogi_camera_input/shogi_camera_input.hpp
  src/game.cpp
  src/img.cpp
  src/move.cpp
  src/piece_book.cpp
  src/position.cpp
  src/random_ai.cpp
  src/session.cpp
  src/shogi_camera_input.cpp
  src/statistics.cpp
  src/sunfish3_ai.cpp
  src/objc/shogi_camera_input.mm
  build/opencv/build/opencv2.xcframework
)
target_include_directories(shogi-camera PUBLIC
  include
  deps/sunfish3/src
  deps/base64/include
)
set_target_properties(shogi-camera PROPERTIES
  XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++20"
  XCODE_ATTRIBUTE_SWIFT_OBJC_INTEROP_MODE "objcxx"
)

if (APPLE)
  add_executable(shogi-camera-demo-ios MACOSX_BUNDLE
    demo/shogi-camera-input-demo/shogi-camera-input-demo/AppDelegate.swift
    demo/shogi-camera-input-demo/shogi-camera-input-demo/DebugPreview.swift
    demo/shogi-camera-input-demo/shogi-camera-input-demo/OpenCV+Utility.swift
    demo/shogi-camera-input-demo/shogi-camera-input-demo/Reader.swift
    demo/shogi-camera-input-demo/shogi-camera-input-demo/SceneDelegate.swift
    demo/shogi-camera-input-demo/shogi-camera-input-demo/ShogiCameraInput+Utility.swift
    demo/shogi-camera-input-demo/shogi-camera-input-demo/ViewController.swift
    demo/shogi-camera-input-demo/shogi-camera-input-demo/Base.lproj
    demo/shogi-camera-input-demo/shogi-camera-input-demo/Assets.xcassets
    demo/shogi-camera-input-demo/shogi-camera-input-demo/Info.plist
    include/shogi_camera_input/module.modulemap
  )
  set_target_properties(shogi-camera-demo-ios PROPERTIES
    BUNDLE True
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.github.kbinani.shogi-camera-demo-ios"
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
    XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++20"
    XCODE_ATTRIBUTE_SWIFT_OBJC_INTEROP_MODE "objcxx"
    XCODE_ATTRIBUTE_INFOPLIST_KEY_UILaunchStoryboardName "LaunchScreen"
    XCODE_ATTRIBUTE_INFOPLIST_KEY_UIMainStoryboardFile "Main"
  )
  target_link_libraries(shogi-camera-demo-ios
    shogi-camera
  )
endif()
