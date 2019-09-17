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

# build update package
chmod +x ./update.sh
./update.sh develop $ver &&
./update.sh demo $ver &&
./update.sh operating $ver
