#!/bin/bash

if [ -z "$ANDROID_NDK" ]; then
  echo "Please set ANDROID_NDK to the Android NDK folder"
  exit 1
fi

if [ -z "$ANDROID_SDK" ]; then
  echo "Please set ANDROID_SDK to the Android SDK folder"
  exit 1
fi

if [ -z "$JAVA_HOME" ]; then
  echo "Please set JAVA_HOME to the Java folder"
  exit 1
fi

JAVAC=$JAVA_HOME/bin/javac
ANDROID_JAR=$ANDROID_SDK/platforms/android-30/android.jar

LIBMEDIAFY_VER=0.1

# Directories, paths and filenames
BUILD_DIR=build_android

CMAKE_ARGS="-H. \
  -DBUILD_SHARED_LIBS=true \
  -DCMAKE_BUILD_TYPE=Release \
  -DANDROID_TOOLCHAIN=clang \
  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
  -DCMAKE_INSTALL_PREFIX=."

function build_libmediafy {

  ABI=$1
  MINIMUM_API_LEVEL=$2
  ABI_BUILD_DIR=build/${ABI}
  STAGING_DIR=staging

  echo "Building Libmediafy for ${ABI}"

  mkdir -p ${ABI_BUILD_DIR} ${ABI_BUILD_DIR}/${STAGING_DIR}

  cmake -B${ABI_BUILD_DIR} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -G Ninja \
        -DANDROID_ABI=${ABI} \
        -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=${STAGING_DIR}/lib/${ABI} \
        -DANDROID_PLATFORM=android-${MINIMUM_API_LEVEL}\
        ${CMAKE_ARGS}

  pushd ${ABI_BUILD_DIR}
  ninja -j5 && ninja install || exit
  popd
}

build_libmediafy armeabi-v7a 21
build_libmediafy arm64-v8a 21
build_libmediafy x86 21
build_libmediafy x86_64 21

# Copy the libs inside the output aar file
pushd output/android
rm -rf libmediafy-$LIBMEDIAFY_VER.aar
mkdir jni
cp -r lib/* jni || exit
zip -ur libmediafy-$LIBMEDIAFY_VER.aar jni || exit
rm -rf jni
popd

# Build the java classes
mkdir -p ${BUILD_DIR}/classes
$JAVAC -cp $ANDROID_JAR android/src/tech/smallwonder/libmediafy/*.java -d ${BUILD_DIR}/classes || exit
cp android/AndroidManifest.xml ${BUILD_DIR}
pushd ${BUILD_DIR}/classes
zip -r ../classes.jar . || exit
popd
pushd ${BUILD_DIR}/
touch R.txt || exit
zip -ur ../output/android/libmediafy-$LIBMEDIAFY_VER.aar AndroidManifest.xml classes.jar R.txt || exit
popd
