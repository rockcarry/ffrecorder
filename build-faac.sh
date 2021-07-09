#!/bin/sh

set -e

TOPDIR=$PWD

if [ ! -d $TOPDIR/faac ]; then
    git clone https://github.com/knik0/faac.git
fi

cd $TOPDIR/faac
git checkout 1_29_9_2
./bootstrap
./configure --prefix=$TOPDIR/libfaac --enable-static --disable-shared --disable-drm
make -j8
make install

cd $TOPDIR
