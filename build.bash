#!/usr/bin/env bash
set -eu
cd "$(dirname ${0})"

function download_cmake () {
  url="$1"
  destdir="$2"
  if [ -d "${destdir}" ]; then
    # already installed
    return
  fi
  # download if not exists
  filename="$(basename "$url")"
  if [ ! -f "${filename}" ]; then
    curl -OL "$url"
  fi
  # install
  ext="${filename##*.}"
  if [ "${ext}" = "zip" ]; then
    unzip -q "${filename}"
    noext="${filename%.*}"
    mv "${noext}" "${destdir}"
  elif [ "${ext}" = "sh" ]; then
    mkdir "${destdir}"
    sh "${filename}" --skip-license --prefix="${destdir}"
  fi
  return
}

function download_ninja () {
  url="$1"
  destdir="$2"
  if [ -d "${destdir}" ]; then
    # already installed
    return
  fi
  # download if not exists
  filename="$(basename "$url")"
  if [ ! -f "${filename}" ]; then
    curl -OL "$url"
  fi
  # install
  mkdir "${destdir}"
  cd "${destdir}"
  cmake -E tar xf "../${filename}"
  cd ..
  return
}

function download_llvm_mingw () {
  url="$1"
  destdir="$2"
  if [ -d "${destdir}" ]; then
    # already installed
    return
  fi
  # download if not exists
  filename="$(basename "$url")"
  if [ ! -f "${filename}" ]; then
    curl -OL "$url"
  fi
  # install
  cmake -E tar xf "${filename}"
  noext="${filename%.*}"
  noext="${noext%.tar}"
  mv "${noext}" "${destdir}"
  return
}

CMAKE_VERSION="3.28.1"
NINJA_VERSION="1.11.1"
LLVM_MINGW_VERSION="20231128"

REBUILD=1
CMAKE_BUILD_TYPE=Release

while [[ $# -gt 0 ]]; do
  case $1 in
    -d|--debug)
      CMAKE_BUILD_TYPE=Debug
      shift
      ;;
    -q|--quick)
      REBUILD=0
      shift
      ;;
    -*|--*)
      echo "Unknown option $1"
      exit 1
      ;;
    *)
      shift
      ;;
  esac
done

case "$(uname -s)" in
  Linux*)
    platform="linux"
    CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh"
    NINJA_URL="https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-linux.zip"
    LLVM_MINGW_URL="https://github.com/mstorsjo/llvm-mingw/releases/download/${LLVM_MINGW_VERSION}/llvm-mingw-${LLVM_MINGW_VERSION}-ucrt-ubuntu-20.04-x86_64.tar.xz"
    ;;
  MINGW64_NT* | MINGW32_NT*)
    platform="windows"
    CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-windows-x86_64.zip"
    NINJA_URL="https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-win.zip"
    LLVM_MINGW_URL="https://github.com/mstorsjo/llvm-mingw/releases/download/${LLVM_MINGW_VERSION}/llvm-mingw-${LLVM_MINGW_VERSION}-ucrt-x86_64.zip"
    ;;
  *)
    echo "Unsupported platform: $(uname -s)"
    exit 1
    ;;
esac

mkdir -p build/tools
cd build/tools

download_cmake ${CMAKE_URL} "cmake-${platform}"
export PATH=$PWD/cmake-${platform}/bin:$PATH
download_ninja ${NINJA_URL} "ninja-${platform}"
export PATH=$PWD/ninja-${platform}:$PATH
download_llvm_mingw ${LLVM_MINGW_URL} "llvm-mingw-${platform}"
export PATH=$PWD/llvm-mingw-${platform}/bin:$PATH

cd ..

for arch in i686 x86_64; do
  destdir="${CMAKE_BUILD_TYPE}/${arch}"
  if [ "${REBUILD}" -eq 1 ]; then
    rm -rf "${destdir}"
    cmake -S .. -B "${destdir}" --preset default \
      -DFORMAT_SOURCES=ON \
      -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
      -DCMAKE_TOOLCHAIN_FILE="cmake/llvm-mingw.cmake" \
      -DCMAKE_C_COMPILER="${arch}-w64-mingw32-clang"
  fi
  cmake --build "${destdir}"
  ctest --test-dir "${destdir}/src/c" --output-on-failure --output-junit testlog.xml
done

destdir="${CMAKE_BUILD_TYPE}"
if [ "${REBUILD}" -eq 1 ]; then
  rm -rf "${destdir}/bin"
fi
mkdir -p "${destdir}/bin"
cp -r "${destdir}/i686/bin/"* "${destdir}/bin"
cp -r "${destdir}/x86_64/bin/script/Extram2.exe" "${destdir}/bin/script"
