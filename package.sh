#!/bin/sh

# clean package folder
rm -rf ./package
mkdir ./package
mkdir ./package/camshare

# package dependents tool
cd deps
./package.sh
cd ..
cp ./deps/package/* ./package/camshare/

# package freeswitch
cd freeswitch
./package.sh
cd ..
cp ./freeswitch/package/* ./package/camshare/

# package CamShareServer
cd CamShareServer
./package.sh
cd ..
cp ./CamShareServer/package/* ./package/camshare/

# copy installl shell
cp ./install.sh ./package/camshare/

# package all
cd package
tar zcvf camshare.tar.gz camshare
rm -rf camshare
cd ..
