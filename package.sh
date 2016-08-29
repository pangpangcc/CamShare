#!/bin/sh

# define param
env=$1

# print param error
param_err="0"
if [ "$env" != "develop" ] && [ "$env" != "demo" ] && [ "$env" != "operating" ]; then
  echo "$env"
  echo "develop"
  param_err="1"
fi
if [ "$param_err" = "1" ]; then
  echo "$0 [develop | demo | operating]"
  exit 0
fi

# clean package folder
rm -rf ./package
mkdir ./package
mkdir ./package/camshare

# package dependents tool
cd deps
chmod +x ./package.sh
./package.sh $env
cd ..
cp ./deps/package/* ./package/camshare/

# package dependents tool rpm packages
cd deps-pkg
chmod +x package.sh
./package.sh $env
cd ..
cp ./deps-pkg/package/* ./package/camshare/

# package freeswitch
cd freeswitch
chmod +x package.sh
./package.sh $env
cd ..
cp ./freeswitch/package/* ./package/camshare/

# package CamShareServer
cd CamShareServer
chmod +x package.sh
./package.sh $env
cd ..
cp ./CamShareServer/package/* ./package/camshare/

# copy installl shell
chmod +x install.sh
cp ./install.sh ./package/camshare/

# enter package folder
cd package

# build package release folder
releasedir="../package_release"
mkdir -p $releasedir

# define release package path
pkgpath="$releasedir/camshare_$env.tar.gz"

# remove old release package file
rm $pkgpath

# package all
tar zcvf $pkgpath camshare
rm -rf camshare
cd ..
