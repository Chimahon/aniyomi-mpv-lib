#!/bin/bash -e

## Dependency versions
# Make sure to keep v_ndk and v_ndk_n in sync, both are listed on the NDK download page

v_sdk=11076708_latest
v_ndk=r27c
v_ndk_n=27.2.12479018
v_sdk_platform=34
v_sdk_build_tools=34.0.0

v_lua=5.2.4
v_unibreak=6.1
v_harfbuzz=10.1.0
v_fribidi=1.0.16
v_freetype=2-13-3
v_fontconfig=2.15.0
v_libass=f61db567e6593df3470e91594bcd4ad2d0473aff
v_mbedtls=3.6.2
v_libxml2=2.13.5
v_ffmpeg=n8.1.2
v_mpv=e7191f2a65d64af266c5c80793e79d2f4b92b789


## Dependency tree
# I would've used a dict but putting arrays in a dict is not a thing

dep_mbedtls=()
dep_libxml2=()
dep_dav1d=()
dep_ffmpeg=(mbedtls dav1d libxml2)
dep_freetype2=()
dep_fribidi=()
dep_harfbuzz=()
dep_unibreak=()
dep_fontconfig=(freetype2 libxml2)
dep_libass=(fontconfig fribidi harfbuzz unibreak)
dep_lua=()
dep_libplacebo=()
dep_mpv=(ffmpeg libass lua libplacebo)
dep_mpv_android=(mpv)


## for CI workflow

# pinned ffmpeg revision
v_ci_ffmpeg=n8.1.2

# filename used to uniquely identify a build prefix
ci_tarball="prefix-ndk-${v_ndk}-lua-${v_lua}-unibreak-${v_unibreak}-harfbuzz-${v_harfbuzz}-fribidi-${v_fribidi}-freetype-${v_freetype}-fontconfig-${v_fontconfig}-mbedtls-${v_mbedtls}-ffmpeg-${v_ci_ffmpeg}.tgz"
