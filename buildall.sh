#!/bin/sh

# compile
./compile.sh &&

# build install package
./packageall.sh &&

# build update package
./updateall.sh
