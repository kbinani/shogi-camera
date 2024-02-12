set -ue

(
  OPENCV_VERSION=4.9.0
  cd "$(dirname "$0")"
  wget https://github.com/opencv/opencv/archive/refs/tags/${OPENCV_VERSION}.zip
  unzip "${OPENCV_VERSION}.zip"
  ln -sf opencv-${OPENCV_VERSION} opencv
  cd "opencv-${OPENCV_VERSION}"
  python ./platforms/apple/build_xcframework.py --out ./build --iphoneos_archs arm64 --iphoneos_deployment_target 14 --build_only_specified_archs --without videoio --without video --without ts --without stitching --without photo --without ml --without highgui --without gapi
)
