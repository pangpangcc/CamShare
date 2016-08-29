#!/bin/sh

# build install package
./package.sh develop &&
./package.sh demo &&
./package.sh operating

