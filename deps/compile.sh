#!/bin/sh

echo "# compile dependence ..."

echo "# compile x264 ..."
tar zxvf x264.tar.gz && cd x264 
./configure --prefix="../build" --enable-static --disable-opencl || exit 1
if [ "$1" == "noclean" ]; then
  echo "# build x264 without clean"
else
  echo "# bulid x264 with clean"
  make clean
fi
make || exit 1
make install-lib-static || exit 1
cd ..

echo "# compile ffmpeg ..."
tar zxvf ffmpeg-2.8.7.tar.gz && cd ffmpeg-2.8.7 
./configure --prefix="../build" --pkg-config-flags="--static" --extra-ldflags="-L../build/lib" --extra-cflags="-I../build/include" --enable-gpl --enable-libx264 || exit 1
if [ "$1" == "noclean" ]; then
  echo "# build ffmpeg without clean"
else
  echo "# bulid ffmpeg with clean"
  make clean
fi
make || exit 1
make install || exit 1
cd ..

