#!/usr/bin/env bash

# If --disable-rust is the next argument, disable Rust code:
RUST_ARG=''
if [ "x${1:-}" = 'x--disable-rust' ]
then
    RUST_ARG='--enable-rust=no'
    shift
fi

PREFIX="$(pwd)/depends/$BUILD/"
HOST="$HOST" BUILD="$BUILD" NO_RUST="$RUST_ARG" "$MAKE" "$@" -C ./depends/ V=1
