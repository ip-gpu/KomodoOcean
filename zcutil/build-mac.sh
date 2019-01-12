#!/bin/bash
export CC=gcc-6
export CXX=g++-6
export LIBTOOL=libtool
export AR=ar
export RANLIB=ranlib
export STRIP=strip
export OTOOL=otool
export NM=nm

set -eu -o pipefail

if [ "x$*" = 'x--help' ]
then
    cat <<EOF
Usage:

$0 --help
  Show this help message and exit.

$0 [ --enable-lcov ] [ MAKEARGS... ]
  Build Zcash and most of its transitive dependencies from
  source. MAKEARGS are applied to both dependencies and Zcash itself. If
  --enable-lcov is passed, Zcash is configured to add coverage
  instrumentation, thus enabling "make cov" to work.
EOF
    exit 0
fi

# If --enable-lcov is the first argument, enable lcov coverage support:
LCOV_ARG=''
HARDENING_ARG='--disable-hardening'
if [ "x${1:-}" = 'x--enable-lcov' ]
then
    LCOV_ARG='--enable-lcov'
    HARDENING_ARG='--disable-hardening'
    shift
fi

TRIPLET=`./depends/config.guess`
PREFIX="$(pwd)/depends/$TRIPLET"

make "$@" -C ./depends/ V=1 NO_QT=1 NO_PROTON=1
./autogen.sh

# QTDIR="/usr/local/Cellar/qt/5.12.0" # fill this with your latest Qt version from Cellar
# cp ${QTDIR}/lib/pkgconfig/*.pc "${PREFIX}/lib/pkgconfig" || echo
# cp /usr/local/Cellar/protobuf/3.6.1.3_1/lib/pkgconfig/*.pc "${PREFIX}/lib/pkgconfig" || echo # don't needed

QTDIR="/usr/local/opt/qt"
cp ${QTDIR}/lib/pkgconfig/*.pc "${PREFIX}/lib/pkgconfig" || echo

CPPFLAGS="-I$PREFIX/include -arch x86_64" LDFLAGS="-L$PREFIX/lib -arch x86_64 -Wl,-no_pie" \
CXXFLAGS='-arch x86_64 -I/usr/local/Cellar/gcc\@6/6.4.0_2/include/c++/6.4.0/ -I$PREFIX/include -fwrapv -fno-strict-aliasing -Werror -g0 -O2 -Wl,-undefined -Wl,dynamic_lookup' \
./configure --prefix="${PREFIX}" --disable-bip70 --with-gui=qt5 "$HARDENING_ARG" "$LCOV_ARG"

make "$@" V=1 NO_GTEST=1 STATIC=1
