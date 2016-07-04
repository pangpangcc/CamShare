#!/bin/sh

# clean package
rm -rf package
mkdir package

# package freeswitch
cd package
cp -rf /usr/local/freeswitch ./
tar -zcvf freeswitch.tar.gz ./freeswitch
rm -rf ./freeswitch
cd ..
