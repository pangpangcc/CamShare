#!/bin/sh

echo "# install dependent tools ..."
sudo yum install -y http://files.freeswitch.org/freeswitch-release-1-6.noarch.rpm epel-release
sudo yum install -y gcc-c++ gdb autoconf automake libtool wget python ncurses-devel zlib-devel libjpeg-devel openssl-devel e2fsprogs-devel sqlite-devel libcurl-devel pcre-devel speex-devel ldns-devel libedit-devel libxml2-devel libyuv-devel opus-devel libvpx-devel libvpx2* libdb4* libidn-devel unbound-devel libuuid-devel lua-devel libsndfile-devel yasm-devel

echo "# decompression freeswitch source ..."
tar zxvf freeswitch.tar.gz &&

echo "# copy custom source to freeswitch ..."
cp -rf ./src/* ./freeswitch/ &&

echo "# compile freeswitch ..."
cd freeswitch &&
chmod +x ./bootstrap.sh &&
./bootstrap.sh &&
chmod +x ./configure &&
./configure --enable-core-odbc-support=no &&
make &&
make install &&
make distclean &&
cd .. &&

echo "# copy freeswitch file ..."
chmod +x ./fs-copy.sh &&
./fs-copy.sh 

echo ""
echo ""
echo "# install finish"
