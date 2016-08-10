#!/bin/sh

# define log file path
log_file="/root/test/close_shell.log"

# set param
h264path=$1
mp4path=$2
jpgpath=$3
userid=$4
siteid=$5
starttime_src=$6
starttime=`date -d "$starttime_src" +%Y%m%d%H%M%S`
starttime_us=`date -d "$starttime_src" +%s`
starttime_year=`date -d "$starttime_src" +%Y`
starttime_month=`date -d "$starttime_src" +%m`
starttime_day=`date -d "$starttime_src" +%d`
endtime=`date +%Y%m%d%H%M%S`
endtime_us=`date +%s`
# print input param to log
#echo "1:$1" >> $log_file
#echo "2:$2" >> $log_file
#echo "3:$3" >> $log_file
#echo "4:$4" >> $log_file
#echo "5:$5" >> $log_file
#echo "6:$6" >> $log_file
#echo "" >> $log_file
#echo "input param, h264path:$h264path, mp4path:$mp4path, jpgpath:$jpgpath, userId:$userid, siteId:$siteid, startTime:$starttime, startTime_src:$starttime_src, startTimeUS:$starttime_us, endTime:$endtime, endTimeUS:$endtime_us" >> $log_file

# build mp4 file
# -- build mp4 file directory
mp4relativedir="$starttime_year/$starttime_month/$starttime_day/"
mp4filedir="$mp4path$mp4relativedir"
if [ ! -d "$mp4filedir" ]; then
  mkdir -p "$mp4filedir"
fi
#echo "$mp4filedir" >> $log_file
# -- build mp4 file 
mp4filename=$userid'_'$starttime'-'$endtime'.mp4'
mp4filepath=$mp4filedir$mp4filename
ffmpeg -i $h264path -y -vcodec copy $mp4filepath > /dev/null 2>&1
#echo "$mp4filepath" >> $log_file
#echo "build mp4 file finish"

# remove jpeg file
rm -f $jpgpath
#echo "remove jpeg file finish"

# request to CamShareServer
#url="http://127.0.0.1:9200/RECORDFINISH?userId=$userid&startTime=$starttime_us&endTime=$endtime_us&siteId=$siteid&fileName=$mp4relativedir$mp4filename"
#http_status=`wget --tries=1 --timeout=3 -O /dev/null -S "$url" 2>&1 | grep "HTTP/" | awk '{print $2}'`
#echo $url >> $log_file
#echo "http_status: $http_status" >> $log_file
#echo "request to CamShareServer finish"

# write result log
# -- request fail
#log_time=`date +'[%Y-%m-%d %H:%M:%S]'`
#if [ "$http_status" != "200" ]; then
#  echo "$log_time request fail, code:$http_status, userId:$userid, siteId:$siteid, startTime:$starttime, endTime:$endtime, fileName:$mp4filename" >> $log_file
#  echo "$log_time $url" >> $log_file
#fi
# -- h264 file not exist
if [ ! -e "$h264path" ]; then
  echo "$log_time h264 file is not exist, userId:$userid, siteId:$siteid, startTime:$starttime, endTime:$endtime, h264Path:$h264path, mp4FileName:$mp4filename, mp4FilePath:$mp4filepath" >> $log_file
fi
# -- mp4 file not exist
if [ ! -e "$mp4filepath" ]; then
  echo "$log_time mp4 file is not exist, userId:$userid, siteId:$siteid, startTime:$starttime, endTime:$endtime, mp4FileName:$mp4filename, mp4FilePath:$mp4filepath" >> $log_file
fi
