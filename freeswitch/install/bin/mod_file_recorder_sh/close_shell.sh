#!/bin/sh

# set param
h264path=$1
mp4path=$2
jpgpath=$3
userid=$4
siteid=$5
starttime=$6
starttime_us=`date -d "$starttime" +%s`
endtime=`date +%Y%m%d%H%M%S`
endtime_us=`date +%s`

# build mp4 file
mp4filepath=$mp4path-$endtime.mp4
mp4filename=$(basename $mp4filepath)
ffmpeg -i $h264path -y -vcodec copy $mp4filepath > /dev/null 2>&1
#echo "build mp4 file finish"

# remove jpeg file
rm -f $jpgpath
#echo "remove jpeg file finish"

# request to CamShareServer
url="http://127.0.0.1:9200/RECORDFINISH?userId=$userid&startTime=$starttime_us&endTime=$endtime_us&siteId=$siteid&fileName=$mp4filename"
http_status=`wget --tries=1 --timeout=3 --spider -S "$url" 2>&1 | grep "HTTP/" | awk '{print $2}'`
#echo "request to CamShareServer finish"

# write log
log_file="../../log/mod_file_recorder.log"
# request fail
log_time=`date +'[%Y-%m-%d %H:%M:%S]'`
if [ "$http_status" != "200" ]; then
  echo "$log_time request fail, code:$http_status, userId:$userid, startTime:$starttime, endTime:$endtime, fileName:$mp4filename" >> $log_file
fi
# h264 file not exist
if [ ! -e "$h264path" ]; then
  echo "$log_time h264 file is not exist, userId:$userid, startTime:$starttime, endTime:$endtime, h264Path:$h264path, mp4FileName:$mp4filename, mp4FilePath:$mp4filepath" >> $log_file
fi
# mp4 file not exist
if [ ! -e "$mp4filepath" ]; then
  echo "$log_time mp4 file is not exist, userId:$userid, startTime:$starttime, endTime:$endtime, mp4FileName:$mp4filename, mp4FilePath:$mp4filepath" >> $log_file
fi
