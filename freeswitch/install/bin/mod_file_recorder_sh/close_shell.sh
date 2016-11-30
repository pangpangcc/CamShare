#!/bin/sh

# define log file path
log_file="/usr/local/freeswitch/log/mod_file_recorder.log"

# define print log function
PrintLog() {
  log_time=`date +'[%Y-%m-%d %H:%M:%S.%N]'`
  echo "$log_time $1" >> $log_file
}

# set param
h264path=$1
mp4path=$2
jpgpath=$3
pich264path=$4
userid=$5
siteid=$6
starttime_src=$7

# print input param to log
#PrintLog "$0 \"$1\" \"$2\" \"$3\" \"$4\" \"$5\" \"$6\" \"$7\""
#PrintLog "1:$1"
#PrintLog "2:$2"
#PrintLog "3:$3"
#PrintLog "4:$4"
#PrintLog "5:$5"
#PrintLog "6:$6"
#PrintLog "7:$7"

# check h264 file
check_h264file=`ls -l $h264path|awk '$5>0'`
if [ -z "$check_h264file" ]; then
  PrintLog "h264 file is not exist or empty, userId:$userid, siteId:$siteid, startTime:$starttime, endTime:$endtime, h264Path:$h264path"
  exit 1
#else
#  PrintLog "h264 file is exist, userId:$userid, siteId:$siteid, startTime:$starttime, endTime:$endtime, h264Path:$h264path"
fi

# define param
starttime=`date -d "$starttime_src" +%Y%m%d%H%M%S`
starttime_us=`date -d "$starttime_src" +%s`
starttime_year=`date -d "$starttime_src" +%Y`
starttime_month=`date -d "$starttime_src" +%m`
starttime_day=`date -d "$starttime_src" +%d`
endtime=`date +%Y%m%d%H%M%S`
endtime_us=`date +%s`
#PrintLog ""
#PrintLog "input param, h264path:$h264path, mp4path:$mp4path, jpgpath:$jpgpath, userId:$userid, siteId:$siteid, startTime:$starttime, startTime_src:$starttime_src, startTimeUS:$starttime_us, endTime:$endtime, endTimeUS:$endtime_us"

# build mp4 file
# -- build mp4 file directory
mp4relativedir="$starttime_year/$starttime_month/$starttime_day/"
mp4filedir="$mp4path$mp4relativedir"
if [ ! -d "$mp4filedir" ]; then
  mkdir -p "$mp4filedir"
fi
#PrintLog "$mp4filedir"
# -- build mp4 file 
mp4filename=$userid'_'$starttime'-'$endtime'.mp4'
mp4filepath=$mp4filedir$mp4filename
tranvideo_cmd="/usr/local/bin/ffmpeg -i $h264path -y -vcodec copy -map 0 -movflags faststart $mp4filepath"
tranvideo_log=`$tranvideo_cmd 2>&1`
#PrintLog "$mp4filepath"
#PrintLog "build mp4 file finish"

# remove jpeg & h264 file
rm -f $jpgpath
rm -f $pich264path

# request to CamShareServer
url="http://127.0.0.1:9200/RECORDFINISH?userId=$userid&startTime=$starttime_us&endTime=$endtime_us&siteId=$siteid&fileName=$mp4relativedir$mp4filename"
http_status=`wget --tries=1 --timeout=3 -O /dev/null -S "$url" 2>&1 | grep "HTTP/" | awk '{print $2}'`
#PrintLog "$url"
#PrintLog "http_status: $http_status"
#PrintLog "request to CamShareServer finish"

# write result log
# -- request fail
if [ "$http_status" != "200" ]; then
  PrintLog "request fail, code:$http_status, userId:$userid, siteId:$siteid, startTime:$starttime, endTime:$endtime, fileName:$mp4filename, url:$url"
fi
# -- h264 file not exist
#if [ ! -e "$h264path" ]; then
#  PrintLog "h264 file is not exist, userId:$userid, siteId:$siteid, startTime:$starttime, endTime:$endtime, h264Path:$h264path, mp4FileName:$mp4filename, mp4FilePath:$mp4filepath, cmd:$tranvideo_cmd"
#fi
# -- mp4 file not exist
if [ ! -e "$mp4filepath" ]; then
  PrintLog "mp4 file is not exist, userId:$userid, siteId:$siteid, startTime:$starttime, endTime:$endtime, mp4FileName:$mp4filename, mp4FilePath:$mp4filepath, cmd:$tranvideo_cmd"
  PrintLog "tranvideo_log:$tranvideo_log"
else
  rm -f $h264path
fi
