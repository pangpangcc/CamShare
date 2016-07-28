#!/bin/sh

# define param
env=$1

# clean package folder
rm -rf package
mkdir package
mkdir ./package/CamShareServer

# package file
cp -rf camshare-middleware ./package/CamShareServer/
if [ "$env" = "develop" ]; then
  cp -rf camshare-middleware.config.develop ./package/CamShareServer/camshare-middleware.config
elif [ "$env" = "demo" ]; then
  cp -rf camshare-middleware.config.demo ./package/CamShareServer/camshare-middleware.config
elif [ "$env" = "operating" ]; then
  cp -rf camshare-middleware.config.operating ./package/CamShareServer/camshare-middleware.config
fi
cp -rf ./bin/* ./package/CamShareServer/
cd package
tar zcvf CamShareServer.tar.gz ./CamShareServer
rm -rf ./CamShareServer
cd ..
