#!/bin/sh

# check param
if [[ -z $1 || -z $2 ]]; then
  echo "./build_days.sh [start date] [end date]"
  echo "./build_days.sh 2016-08-01 2016-08-07"
  exit
fi

startdate=$1
enddate=$2
start_timestamp=`date -d "$startdate" +%s`
if [ $? -ne 0 ]; then
  echo "start date [$startdate] error, please retry!"
  exit
fi

end_timestamp=`date -d "$enddate" +%s`
if [ $? -ne 0 ]; then
  echo "end date [$enddate] error, please retry!"
  exit
fi

if [[ $start_timestamp -gt $end_timestamp ]]; then
  echo "error date, please retry!"
  echo "start:$startdate, end:$enddate"
  exit
fi

# define param
srcdirpath="/home/samson/log/users/log/"
dstdirpath="/home/samson/log/users/log/week/"
srcfileext=".txt"
dstfileext=".csv"
dstfile=$startdate"_"$enddate$dstfileext
dstpath=$dstdirpath$dstfile
dsttempext=".tmp"
dsttemppath=$dstpath$dsttempext

# remove temp file
if [ -e $dstpath ]; then
  rm $dstpath
fi
if [ -e $dsttemppath ]; then
  rm $dsttemppath
fi

# get log
currdate=$startdate
while true; do
  curr_timestamp=`date -d "$currdate" +%s`
  if [ $end_timestamp -ge $curr_timestamp ]; then
    # build day log
    month=`date -d "$currdate" +%m`
    day=`date -d "$currdate" +%d`
    ./build_users.sh $month $day

    # build week log
    currpath="$srcdirpath$currdate$srcfileext"
    if [ -e $currpath ]; then
      cat $currpath >> $dsttemppath
    fi
    currdate=`date -d "$currdate +1 day" +%Y-%m-%d`
  else
    break;
  fi
done

# sort
sort -u $dsttemppath > $dstpath

# remove temp file
rm $dsttemppath

# print
echo "finish, path:$dstpath"
