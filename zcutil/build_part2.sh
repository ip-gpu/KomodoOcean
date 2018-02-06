#!/usr/bin/env bash

set -eu -o pipefail

# Allow user overrides to $MAKE. Typical usage for users who need it:
#   MAKE=gmake ./zcutil/build.sh -j$(nproc)
if [[ -z "${MAKE-}" ]]; then
    MAKE=make
fi

# Allow overrides to $BUILD and $HOST for porters. Most users will not need it.
#   BUILD=i686-pc-linux-gnu ./zcutil/build.sh
if [[ -z "${BUILD-}" ]]; then
    BUILD=x86_64-unknown-linux-gnu
fi
if [[ -z "${HOST-}" ]]; then
    HOST=x86_64-unknown-linux-gnu
fi

# Allow override to $CC and $CXX for porters. Most users will not need it.
if [[ -z "${CC-}" ]]; then
    CC=gcc
fi
if [[ -z "${CXX-}" ]]; then
    CXX=g++
fi

if [ "x$*" = 'x--help' ]
then
    cat <<EOF
Usage:

$0 --help
  Show this help message and exit.

$0 [ --enable-lcov || --disable-tests ] [ --disable-mining ] [ --disable-rust ] [ MAKEARGS... ]
  Build Zcash and most of its transitive dependencies from
  source. MAKEARGS are applied to both dependencies and Zcash itself.

  If --enable-lcov is passed, Zcash is configured to add coverage
  instrumentation, thus enabling "make cov" to work.
  If --disable-tests is passed instead, the Zcash tests are not built.

  If --disable-mining is passed, Zcash is configured to not build any mining
  code. It must be passed after the test arguments, if present.

  If --disable-rust is passed, Zcash is configured to not build any Rust language
  assets. It must be passed after mining/test arguments, if present.
EOF
    exit 0
fi

set -x
cd "$(dirname "$(readlink -f "$0")")/.."

# If --enable-lcov is the first argument, enable lcov coverage support:
LCOV_ARG=''
HARDENING_ARG='--enable-hardening'
TEST_ARG=''
if [ "x${1:-}" = 'x--enable-lcov' ]
then
    LCOV_ARG='--enable-lcov'
    HARDENING_ARG='--disable-hardening'
    shift
elif [ "x${1:-}" = 'x--disable-tests' ]
then
    TEST_ARG='--enable-tests=no'
    shift
fi

# If --disable-mining is the next argument, disable mining code:
MINING_ARG=''
if [ "x${1:-}" = 'x--disable-mining' ]
then
    MINING_ARG='--enable-mining=no'
    shift
fi

# If --disable-rust is the next argument, disable Rust code:
RUST_ARG=''
if [ "x${1:-}" = 'x--disable-rust' ]
then
    RUST_ARG='--enable-rust=no'
    shift
fi

PREFIX="$(pwd)/depends/$BUILD/"

HOST="$HOST" BUILD="$BUILD" NO_RUST="$RUST_ARG" "$MAKE" "$@" -C ./depends/ V=1

./autogen.sh
DECKER_ARGS="--enable-tests=no --enable-wallet=yes with_boost_libdir=$(pwd)/depends/x86_64-unknown-linux-gnu/lib"
DECKER_QT_INCPATH='-isystem /usr/include/x86_64-linux-gnu/qt5 -isystem /usr/include/x86_64-linux-gnu/qt5/QtWidgets -isystem /usr/include/x86_64-linux-gnu/qt5/QtGui -isystem /usr/include/x86_64-linux-gnu/qt5/QtNetwork -isystem /usr/include/x86_64-linux-gnu/qt5/QtDBus -isystem /usr/include/x86_64-linux-gnu/qt5/QtCore'
#CPPFLAGS="-I$(pwd)/depends/x86_64-unknown-linux-gnu/include/" LDFLAGS="-L$(pwd)/depends/x86_64-unknown-linux-gnu/lib/"
#BDB_CPPFLAGS=-I$(pwd)/depends/x86_64-unknown-linux-gnu/include BDB_LIBS=-L$(pwd)/depends/x86_64-unknown-linux-gnu/lib/

DECKER_DEPS="CPPFLAGS=-I$(pwd)/depends/x86_64-unknown-linux-gnu/include LDFLAGS=-L$(pwd)/depends/x86_64-unknown-linux-gnu/lib"

# with prefix
#CC="$CC" CXX="$CXX" ./configure --prefix="${PREFIX}" --host="$HOST" --build="$BUILD" "$RUST_ARG" "$HARDENING_ARG" "$LCOV_ARG" "$TEST_ARG" "$MINING_ARG" CXXFLAGS='-fwrapv -fno-strict-aliasing -Werror -g' "$DECKER_ARGS"
# example of adding specific BDB
# 	./configure BDB_CFLAGS="-I${BDB_PREFIX}/include" BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx"
# example of adding specific BDB via LD and CPP flags
#  	./configure LDFLAGS="-L${BDB_PREFIX}/lib/" CPPFLAGS="-I${BDB_PREFIX}/include/"
# same with CONFIG_SITE
# 	CONFIG_SITE=/home/build/straks/depends/x86_64-w64-mingw32/share/config.site ./configure LDFLAGS="-L${BDB_PREFIX}/lib/" CPPFLAGS="-I${BDB_PREFIX}/include/" --prefix=/
# example of adding includes and libs
#	CPPFLAGS="-I/path1 -I/path2" LDFLAGS="-L/path1 -L/path2"

echo -e "\n"
CC="$CC" CXX="$CXX" ./configure CXXFLAGS='-fPIC -fwrapv -fno-strict-aliasing -Werror -g' $DECKER_ARGS $DECKER_DEPS

"$MAKE" "$@" V=1
