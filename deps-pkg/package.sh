#!/bin/sh

# clean package folder
rm -rf package
mkdir package
mkdir ./package/deps-pkg

# package file
cp -f ./*.rpm ./package/deps-pkg/
cd package
tar zcvf deps-pkg.tar.gz ./deps-pkg
rm -rf ./deps-pkg
cd ..

