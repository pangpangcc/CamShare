#!/bin/sh

if [[ -z $1 || -z $2 ]]; then
  echo "./build_users.sh [month] [day]"
  echo "./build_users.sh 08 07"
  exit
fi

month=$1
day=$2

dstpath="/home/samson/log/users/log/"
dstfile="2016-$month-$day.txt"
dstfilepath=$dstpath$dstfile

# build file if file is not exist
if [ ! -e $dstfilepath ]; then
  # get login users id
  dsttempext=".tmp"
  dsttemppath=$dstfilepath$dsttempext
  `grep "is now logged into" /usr/local/freeswitch/log/freeswitch.*|grep "2016-$month-$day"|awk '{print $13}'|awk -F @ '{print $1}' > $dsttemppath`

  # sort & unique
  sort -u $dsttemppath > $dstfilepath
  
  # remove temp file
  rm -f $dsttemppath

  # print
  echo "$dstfilepath OK!"
fi
