#!/bin/sh

# define param
ver=$1

# print param error
param_err="0"
if [ -z "$ver" ]; then
  param_err="1"
fi
if [ "$param_err" != "0" ]; then
  echo "$0 version"
  exit 0
fi

# build install package
chmod +x ./package.sh
./package.sh develop $ver &&
./package.sh demo $ver &&
./package.sh operating $ver

