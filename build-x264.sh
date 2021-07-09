#!/bin/sh

set -e

TOPDIR=$PWD

if [ ! -d $TOPDIR/x264 ]; then
    git clone https://github.com/mirror/x264.git
fi

cd $TOPDIR/x264
git checkout stable
./configure \
--prefix=$TOPDIR/libx264 \
--disable-cli \
--disable-shared \
--enable-static \
--disable-interlaced \
--bit-depth=8 \
--chroma-format=420 \
--enable-lto \
--enable-strip \
--enable-pic \
--disable-opencl \
--disable-avs \
--disable-swscale \
--disable-lavf \
--disable-ffms \
--disable-gpac \
--disable-lsmash
make -j8
make install

cd $TOPDIR
