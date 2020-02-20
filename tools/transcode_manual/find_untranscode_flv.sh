#!/bin/sh
# Find untranscode flv script
# Author: Max.Chiu
# Date: 2020/02/19

function Usage {
	echo "Usage : ./find_untranscode_flv.sh [DATE_FOR_MONTH]"
	echo "Usage : ./find_untranscode_flv.sh 202002"
}

DATE_WITH_MONTH=""
if [ ! "$1" == "" ]
then
	DATE_WITH_MONTH="$1"
else
	Usage;
	exit 1;
fi

FLV_ROOT_DIR=/app/freeswitch/recordings/video_h264
MP4_ROOT_DIR=/app/freeswitch/recordings/video_mp4
MONTH_DIR=$(date -d "${DATE_WITH_MONTH}01" +%Y/%m) || exit 1

FLV_MONTH_DIR=$FLV_ROOT_DIR/$MONTH_DIR
MP4_MONTH_DIR=$MP4_ROOT_DIR/$MONTH_DIR

FLV_DAY_DIR_ARRAY=($(ls $FLV_MONTH_DIR))
MP4_DAY_DIR_ARRAY=($(ls $MP4_MONTH_DIR))
#echo -e "FLV_DAY_DIR_ARRAY_COUNT:${#FLV_DAY_DIR_ARRAY[@]}"
#echo -e "MP4_DAY_DIR_ARRAY_COUNT:${#MP4_DAY_DIR_ARRAY[@]}"

rm result_flv.txt -rf
if [ ${#FLV_DAY_DIR_ARRAY[@]} -ge ${#MP4_DAY_DIR_ARRAY[@]} ];then
  for((day=0;day<${#FLV_DAY_DIR_ARRAY[@]};day++));do
	  FLV_DAY_DIR=$FLV_MONTH_DIR/${FLV_DAY_DIR_ARRAY[$day]}
	  MP4_DAY_DIR=$MP4_MONTH_DIR/${MP4_DAY_DIR_ARRAY[$day]}
	  
	  echo -e "# $FLV_DAY_DIR [\033[33mCheck\033[0m]"
	  #echo -e "# FLV_DAY_DIR:$FLV_DAY_DIR"
	  #echo -e "# MP4_DAY_DIR:$MP4_DAY_DIR"
	  
	  # Find flv list
    FLV_COUNT=$(find $FLV_DAY_DIR -type f -size +13c | xargs ls -al | wc -l)
    FLV_LIST=($(find $FLV_DAY_DIR -type f -size +13c | xargs ls -al | awk -F ' ' '{print $9}'))

    # Find mp4 list
    MP4_COUNT=$(find $MP4_DAY_DIR -type f | xargs ls -al | wc -l)
    MP4_LIST=($(find $MP4_DAY_DIR -type f -size +13c | xargs ls -al | awk -F ' ' '{print $9}'))

    if [ $FLV_COUNT -gt $MP4_COUNT ]; then
      echo -e "$FLV_DAY_DIR [\033[31mDiff\033[0m] FLV_COUNT:[\033[33m$FLV_COUNT\033[0m] MP4_COUNT:[\033[33m$MP4_COUNT\033[0m]"
      for((i=0,j=0;i<${#FLV_LIST[@]};i++,j++));do
        FLV_ORINAL_NAME=${FLV_LIST[$i]}
        MP4_ORINAL_NAME=${MP4_LIST[$j]}
        FLV_NAME=$(echo $FLV_ORINAL_NAME | awk -F '/' '{print $9}' | awk -F '.' '{print $1}' | awk -F '_' '{print $1"_"$3}')
        MP4_NAME=$(echo $MP4_ORINAL_NAME | awk -F '/' '{print $9}' | awk -F '-' '{print $1}')
        #echo -e "# i:$i FLV_NAME:$FLV_NAME MP4_NAME:$MP4_NAME"
    
        if [ $FLV_NAME != $MP4_NAME ];then
          echo -e "# [\033[31mFound\033[0m] FLV_ORINAL_NAME:$FLV_ORINAL_NAME"
          echo $FLV_ORINAL_NAME >> result_flv.txt
    	    ((i++))
        fi
      done
    else
      echo -e "# $FLV_DAY_DIR [\033[32mOK\033[0m]"
    fi
  done
fi