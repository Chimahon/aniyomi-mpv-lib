#!/bin/bash -e

. ../../include/path.sh

build=_build$ndk_suffix

if [ "$1" == "build" ]; then
	true
elif [ "$1" == "clean" ]; then
	rm -rf "$build"
	exit 0
else
	exit 255
fi

mkdir -p "$build"
cd "$build"

../configure \
	--host="$ndk_triple" \
	--enable-static --disable-shared --with-pic \
	--enable-libxml2 --disable-docs --disable-nls --disable-cache-build \
	--with-default-fonts=/system/fonts,/product/fonts \
	--with-add-fonts=no

make -j$cores
make DESTDIR="$prefix_dir" install
