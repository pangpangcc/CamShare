#!/bin/sh

# clean package
rm -rf package
mkdir package
mkdir package/usr_local_bin

# package file
cp ./build/bin/* ./package/usr_local_bin/
cd package
tar -zcvf usr_local_bin.tar.gz ./usr_local_bin
rm -rf ./usr_local_bin
cd ..
