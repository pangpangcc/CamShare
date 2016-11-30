#!/bin/sh

# define param
ver=$1

# check param
param_err="0"
if [ -z "$ver" ]; then
  param_err="1"
fi
if [ "$param_err" != "0" ]; then
  echo "$0 version"
  exit 0
fi

# compile
./compile.sh &&

# build install package
./packageall.sh $ver &&

# build update package
./updateall.sh $ver
