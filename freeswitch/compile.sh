#!/bin/sh

echo "# install dependent tools ..."
sudo yum install -y http://files.freeswitch.org/freeswitch-release-1-6.noarch.rpm epel-release
sudo yum install -y gcc-c++ gdb autoconf automake libtool wget python ncurses-devel zlib-devel libjpeg-devel openssl-devel e2fsprogs-devel sqlite-devel libcurl-devel pcre-devel speex-devel ldns-devel libedit-devel libxml2-devel libyuv-devel opus-devel libvpx-devel libvpx2* libdb4* libidn-devel unbound-devel libuuid-devel lua-devel libsndfile-devel yasm-devel libpng-devel
REBUILDALL=0
if [ -d "freeswitch" ]; then
  echo "# dont decompression freeswitch source ..."
else
  echo "# decompression freeswitch source ..."
  REBUILDALL=1
  tar zxvf freeswitch.tar.gz || exit 1
fi

echo "# copy custom source to freeswitch ..."
cp -rf ./src/* ./freeswitch/ || exit 1

echo "# compile freeswitch ..."
cd freeswitch
if [ $REBUILDALL == 0 -a "$1" == "noclean" ]; then
  echo "# build freeswitch without clean"
else
  echo "# bulid freeswitch with clean"
  chmod +x ./bootstrap.sh || exit 1
  ./bootstrap.sh || exit 1
  chmod +x ./configure || exit 1
  ./configure --enable-core-odbc-support=no || exit 1
  
  if [ ! -f "first_build_test" ]; then
    touch first_build_test
  else
    make clean
  fi
fi

make || exit 1
make install || exit 1
cd ..

echo "# copy freeswitch file ..."
chmod +x ./fs-copy.sh || exit 1
./fs-copy.sh || exit 1

echo ""
echo ""
echo "# install finish"
