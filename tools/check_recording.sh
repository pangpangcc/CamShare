#!/bin/sh

check_recording()
{
  LOG_FILE_PATH="./check_recording.tmp.log"

  RECORD_FOLDER=/app/freeswitch/recordings
  H264_FOLDER=$RECORD_FOLDER/video_h264
  MP4_FOLDER=$RECORD_FOLDER/video_mp4

  TODAY_YEAR=`date +"%Y"`
  TODAY_MONTH=`date +"%m"`
  TODAY_DAY=`date +"%d"`

  TODAY_H264_FOLDER="$H264_FOLDER/$TODAY_YEAR/$TODAY_MONTH/$TODAY_DAY"
  H264_VIDEOS=`ls -lt $TODAY_H264_FOLDER|grep -v "total"|awk '{print $9}'`
  #echo $TODAY_H264_FOLDER

  TODAY_MP4_FOLDER="$MP4_FOLDER/$TODAY_YEAR/$TODAY_MONTH/$TODAY_DAY"
  #echo $TODAY_MP4_FOLDER

  FIRST_NOMP4_H264_FILE=""
  while read -r H264_FILE
  do
    FIND_KEY=`echo "$H264_FILE"|awk -F '_' '{print $1"_"$3}'|awk -F '.' '{print $1}'`
    MP4_FILE=`ls $TODAY_MP4_FOLDER/$FIND_KEY* 2>/dev/null`
    if [ -z "$MP4_FILE" ]; then
      VIDEO_DURATION=$(ffmpeg -i "$TODAY_H264_FOLDER/$H264_FILE" 2>&1|grep Duration)
      if [ -n "$VIDEO_DURATION" ]; then
        FIRST_NOMP4_H264_FILE="$H264_FILE"
      fi
    else
      break
    fi
  done <<< "$H264_VIDEOS"

  if [ -n "$FIRST_NOMP4_H264_FILE" ]; then
    H264_MODIFY_TIME=`ls -l --time-style="+%s" $TODAY_H264_FOLDER/$FIRST_NOMP4_H264_FILE|awk '{print $6}'`
    NOW_TIME=`date +%s`
    DIFF_TIME=$(($NOW_TIME-$H264_MODIFY_TIME))
    if [ $DIFF_TIME -gt 100 ]; then
      LOG_TIME=`date +"%Y-%m-%d %H:%M:%S"`

      # write mail file
      VIDEO_FILE_SIZE=`ls -lh $TODAY_H264_FOLDER/$FIRST_NOMP4_H264_FILE|awk '{print $5}'`
      echo "Subject: CamShare Streaming Server: transcode timeout" > $LOG_FILE_PATH
      echo "" >> $LOG_FILE_PATH
      echo "[ $LOG_TIME ] warning: $TODAY_H264_FOLDER/$FIRST_NOMP4_H264_FILE file_size:$VIDEO_FILE_SIZE diff_time:$DIFF_TIME" >> $LOG_FILE_PATH

      # send mail
      sendmail samson.fan@qpidnetwork.com < $LOG_FILE_PATH
      #sendmail davidhu@qpidnetwork.com < $LOG_FILE_PATH
      #cat $LOG_FILE_PATH
  
      # remove file
      rm -f $LOG_FILE_PATH
    fi
  fi
}


SLEEP_SEC=900
while true
do
  check_recording
  sleep $SLEEP_SEC
done

