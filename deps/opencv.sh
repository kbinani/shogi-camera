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
  WITHOUT_FLAGS="--without calib3d --without dnn --without gapi --without java --without js --without ml --without objdetect --without photo --without python --without stitching --without ts --without video --without videoio"
  DISABLE_FLAGS="--disable JPEG --disable WEBP --disable SPNG --disable IMGCODEC_HDR --disable IMGCODEC_SUNRASTER --disable IMGCODEC_PXM --disable IMGCODEC_PFM"
  python ./platforms/apple/build_xcframework.py --out ./build-ios --disable-bitcode --iphoneos_archs arm64 --iphoneos_deployment_target 15 --build_only_specified_archs $WITHOUT_FLAGS $DISABLE_FLAGS
  python ./platforms/osx/build_framework.py --out ./build-macos $WITHOUT_FLAGS $DISABLE_FLAGS
)
