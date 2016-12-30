#!/bin/sh

# define param
ver=$1
noclean="$2"

# check param
param_err="0"
if [ -z "$ver" ]; then
  param_err="1"
fi
if [ "$param_err" != "0" ]; then
  echo "$0 version"
  exit 0
fi

if [ "$noclean" == "noclean" ]; then
	echo "# build withou clean"
else
  echo "# build with clean"
  noclean=""
fi

# compile
./compile.sh $noclean &&

# build install package
./packageall.sh $ver &&

# build update package
./updateall.sh $ver
