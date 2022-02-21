#!/usr/bin/env bash

HOST=x86_64-w64-mingw32
CXX=x86_64-w64-mingw32-g++-posix
CC=x86_64-w64-mingw32-gcc-posix
PREFIX="$(pwd)/depends/$HOST"

set -eu -o pipefail

set -x
cd "$(dirname "$(readlink -f "$0")")/.."

make "$@" -C ${PWD}/depends V=1 HOST=x86_64-w64-mingw32
./autogen.sh
CONFIG_SITE="$PWD/depends/x86_64-w64-mingw32/share/config.site" CXXFLAGS="-DCURL_STATICLIB -g0 -O2" ./configure --disable-tests --disable-bench --with-gui=qt5 --disable-bip70
make "$@" V=1
