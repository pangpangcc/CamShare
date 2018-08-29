#!/bin/sh
# 统计14天内数据 (短视频 < 500k)
# Author:	Max.Chiu

TIME=$(date +%s)
DAYS=14
i=0
while [[ $i -lt $DAYS ]];do
	TIME_FORMAT=$(date -d @$TIME +%Y/%m/%d)
	VIDEO_PATH="/usr/local/freeswitch/recordings/video_h264/$TIME_FORMAT"
	
	# 视频统计
	VIDEO_TOTAL_SIZE=0
	VIDEO_NORMAL_COUNT=0
	VIDEO_SHORT_COUNT=0
	
	if [ -x "$VIDEO_PATH" ];then
		VIDEO_TOTAL_SIZE=$(du -sh $VIDEO_PATH | awk -F ' ' '{print $1}')
		VIDEO_NORMAL_COUNT=$(ls -al $VIDEO_PATH | awk -F ' ' '{if($5>=500*1024)print $5,$9}' | wc -l)
		VIDEO_SHORT_COUNT=$(ls -al $VIDEO_PATH | awk -F ' ' '{if($5<500*1024)print $5,$9}' | wc -l)
	fi
	
	# Makecall统计
	TIME_FORMAT_LOG=$(date -d @$TIME +%Y-%m-%d)
	TIME_FORMAT_LOG_FILE=$(date -d @$TIME +%Y%m)
	MAKECALL_COUNT=$(grep -e "$TIME_FORMAT_LOG" /usr/local/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "Freeswitch-事件处理-增加会议室成员" | wc -l)
	TIMEOUT_COUNT=$(grep -e "$TIME_FORMAT_LOG" /usr/local/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "外部服务(LiveChat)-收到命令:进入会议室认证结果-失败-找不到任务-已经超时" | grep "ACTIVE" | wc -l)
	FAIL_COUNT=$(grep -e "$TIME_FORMAT_LOG" /usr/local/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "外部服务(LiveChat)-收到命令:进入会议室认证结果-失败" | wc -l)
 	FAIL_RADIO=$(echo | awk '{print fail/total}' fail=$FAIL_COUNT total=$MAKECALL_COUNT)
  printf "$TIME_FORMAT TotalVideoSize:\033[32m%5s\033[0m, NormalVideoCount:\033[33m%5s\033[0m, ShortVideoCount:\033[31m%5s\033[0m, Makecall:\033[36m%5s\033[0m, MakecallTimeout:\033[35m%5s\033[0m, MakecallFail:\033[31m%5s\033[0m, MakecallFailRadio:\033[36m%.6s\033[0m \n" $VIDEO_TOTAL_SIZE $VIDEO_NORMAL_COUNT $VIDEO_SHORT_COUNT $MAKECALL_COUNT $TIMEOUT_COUNT $FAIL_COUNT $FAIL_RADIO

	((i++))
	((TIME=$TIME-(24*3600)))
	
done
