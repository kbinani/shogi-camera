set -ue

(
  OPENCV_VERSION=4.9.0
  cd "$(dirname "$0")/opencv"
  if [ ! -f "${OPENCV_VERSION}.zip" ]; then
    wget https://github.com/opencv/opencv/archive/refs/tags/${OPENCV_VERSION}.zip
  fi
  rm -rf "opencv-${OPENCV_VERSION}"
  unzip "${OPENCV_VERSION}.zip"
  ln -sf opencv-${OPENCV_VERSION} opencv
  cd "opencv-${OPENCV_VERSION}"
  python ./platforms/apple/build_xcframework.py --out ./build-ios --iphoneos_archs arm64 --iphonesimulator_archs x86_64,arm64 --iphoneos_deployment_target 15 --build_only_specified_archs --without videoio --without video --without ts --without stitching --without photo --without ml --without gapi
  python ./platforms/osx/build_framework.py --out ./build-macos --without videoio --without video --without ts --without stitching --without photo --without ml --without gapi
)
