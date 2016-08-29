#!/bin/sh

echo "# install dependent tools ..."
sudo yum install -y libidn-devel

# set ./bin/* executable
chmod +x ./bin/*

# compile
chmod +x ./configure
./configure &&
make
