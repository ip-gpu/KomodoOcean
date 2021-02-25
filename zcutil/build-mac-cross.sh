#!/usr/bin/env bash

# don't forget to place SDK into project folder before run this script
mkdir -p ${PWD}/depends/SDKs
tar -C ${PWD}/depends/SDKs -xf ${PWD}/Xcode-11.3.1-11C505-extracted-SDK-with-libcxx-headers.tar.gz
# make deps
make -C ${PWD}/depends v=1 NO_PROTON=1 HOST=x86_64-apple-darwin18 DARWIN_SDK_PATH=${PWD}/depends/SDKs/Xcode-11.3.1-11C505-extracted-SDK-with-libcxx-headers -j$(nproc --all)
./autogen.sh
LDFLAGS="-Wl,-no_pie" \
CONFIG_SITE="$PWD/depends/x86_64-apple-darwin18/share/config.site" ./configure --disable-tests --disable-bench --with-gui=qt5 --disable-bip70
# make app
make V=1 -j$(nproc --all)