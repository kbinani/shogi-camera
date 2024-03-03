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
  DISABLE="--without calib3d --without dnn --without gapi --without java --without js --without ml --without objdetect --without photo --without python --without stitching --without ts --without video --without videoio"
  python ./platforms/apple/build_xcframework.py --out ./build-ios --iphoneos_archs arm64 --iphoneos_deployment_target 15 --build_only_specified_archs $DISABLE
  python ./platforms/osx/build_framework.py --out ./build-macos $DISABLE
)
