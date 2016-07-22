#!/bin/sh

# clean package folder
rm -rf ./package
mkdir ./package
mkdir ./package/camshare

# package dependents tool
cd deps
chmod +x ./package.sh
./package.sh
cd ..
cp ./deps/package/* ./package/camshare/

# package dependents tool rpm packages
cd deps-pkg
chmod +x package.sh
./package.sh
cd ..
cp ./deps-pkg/package/* ./package/camshare/

# package freeswitch
cd freeswitch
chmod +x package.sh
./package.sh
cd ..
cp ./freeswitch/package/* ./package/camshare/

# package CamShareServer
cd CamShareServer
chmod +x package.sh
./package.sh
cd ..
cp ./CamShareServer/package/* ./package/camshare/

# copy installl shell
chmod +x install.sh
cp ./install.sh ./package/camshare/

# package all
cd package
tar zcvf camshare.tar.gz camshare
rm -rf camshare
cd ..
