#!/bin/sh

# clean package folder
rm -rf package
mkdir package
mkdir ./package/CamShareServer

# package file
cp -rf camshare-middleware ./package/CamShareServer/
cp -rf camshare-middleware.config ./package/CamShareServer/
cp -rf ./bin/* ./package/CamShareServer/
cd package
tar zcvf CamShareServer.tar.gz ./CamShareServer
rm -rf ./CamShareServer
cd ..
