#!/bin/sh

echo "# compile dependence ..."

echo "# compile x264 ..."
tar zxvf x264.tar.gz && cd x264
./configure --prefix="../build" --enable-static --disable-opencl
make
make install-lib-static
make distclean
cd ..

echo "# compile ffmpeg ..."
tar zxvf ffmpeg-2.8.7.tar.gz && cd ffmpeg-2.8.7
./configure --prefix="../build" --pkg-config-flags="--static" --extra-ldflags="-L../build/lib" --extra-cflags="-I../build/include" --enable-gpl --enable-libx264
make
make install
make distclean
cd ..

