#!/bin/sh
# 补转码和提交Camshare记录

FILE=$1

# [[ -n $line ]] prevents the last line from being ignored if it doesn't end with a \n (since  read returns a non-zero exit code when it encounters EOF).
while IFS= read -r LINE || [[ -n "$LINE" ]]; do

#echo $LINE
DIR=`dirname $LINE`
FILENAME=`echo $LINE | awk -F '/' '{print $NF}'`
USERID=`echo $FILENAME | awk -F '_' '{print $1}'`
SITEID=`echo $FILENAME | awk -F '_' '{print $2}'`

TIME=`ffmpeg -i $LINE 2>&1 | grep Duration | cut -d ' ' -f 4 | sed s/,// | awk -F '.' '{print $1}'`
if [ -z "$TIME" ] || [ "$TIME" == "N/A" ]; then
	echo "Skip $LINE" >> skip.log
	continue
fi

# 获取视频时长
HOUR=`echo $TIME | awk -F ':' '{print $1}'`
MIN=`echo $TIME | awk -F ':' '{print $2}'`
SEC=`echo $TIME | awk -F ':' '{print $3}'`

# 10进制转换
HOUR=$[10#$HOUR]
MIN=$[10#$MIN]
SEC=$[10#$SEC]
# 生成时长的(秒数)
DURATION=$[$HOUR*3600+$MIN*60+$SEC];

# 生成时间戳和日期格式
FILE_START_TIME=`echo $FILENAME | awk -F '_' '{print $3}' | awk -F '.' '{print $1}'`
START_YEAR=`echo $FILE_START_TIME | cut -c 1-4`
START_MON=`echo $FILE_START_TIME | cut -c 5-6`
START_DAY=`echo $FILE_START_TIME | cut -c 7-8`
START_HOUR=`echo $FILE_START_TIME | cut -c 9-10`
START_MIN=`echo $FILE_START_TIME | cut -c 11-12`
START_SEC=`echo $FILE_START_TIME | cut -c 13-14`

START_TIME_STRING="$START_YEAR-$START_MON-$START_DAY $START_HOUR:$START_MIN:$START_SEC"
START_TIME=`date -d "$START_TIME_STRING" +%s`

END_TIME=$[$START_TIME+$DURATION]
FILE_END_TIME=`date -d "@$END_TIME" +%Y%m%d%H%M%S`

MP4_DIR=`echo $DIR | sed s/h264/mp4/`
MP4="$MP4_DIR/$USERID"_"$FILE_START_TIME"-"$FILE_END_TIME.mp4"
MP4_FILENAME=`echo $MP4 | sed s/.*video_mp4[/]//g`
JPG="/app/freeswitch/recordings/pic_jpg/$USERID.jpg"
H264="/app/freeswitch/recordings/pic_h264/$USERID.jpg"

# transcode
CMD="/usr/local/bin/ffmpeg -i $LINE -loglevel error -y -vcodec copy -map 0 -movflags faststart $MP4"
echo "$CMD" >> transcode.log
RES=$($CMD </dev/null 2>&1)
echo "$RES" >> transcode.log

# request to CamShareServer
URL="http://127.0.0.1:9200/RECORDFINISH?userId=$USERID&startTime=$START_TIME&endTime=$END_TIME&siteId=$SITEID&fileName=$MP4_FILENAME"
echo $URL >> transcode.log
RES=`wget --tries=1 --timeout=3 -O /dev/null -S "$URL" 2>&1 | grep "HTTP/" | awk '{print $2}'`
echo "HTTP:$RES" >> transcode.log

echo "sleep 2" >>transcode.log
sleep 2

done <"$FILE"
