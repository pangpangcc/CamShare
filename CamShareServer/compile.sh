#!/bin/sh

echo "# install dependent tools ..."
sudo yum install -y libidn-devel

# compile
chmod +x ./configure
./configure
make
