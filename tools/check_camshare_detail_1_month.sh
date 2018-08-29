#!/bin/sh
# 统计30天内数据 (短视频 < 500k)
TIME=$(date +%s)
DAYS=30
i=0
while [[ $i -lt $DAYS ]];do
	TIME_FORMAT=$(date -d @$TIME +%Y/%m/%d)
	TIME_FORMAT_SHOW=$(date -d @$TIME +%Y/%m/%d:%a)
	VIDEO_PATH="/app/freeswitch/recordings/video_h264/$TIME_FORMAT"
	
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
	MAKECALL_COUNT=$(grep -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "Freeswitch-事件处理-增加会议室成员\|Freeswitch-事件处理-websocket终端登录" | wc -l)
	MAKECALL_MAN_COUNT=$(grep -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "Freeswitch-事件处理-增加会议室成员], user : CM\|Freeswitch-事件处理-websocket终端登录, user : CM" | wc -l)
	TIMEOUT_COUNT=$(grep -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "外部服务(LiveChat)-收到命令:进入会议室认证结果-失败-找不到任务-已经超时" | grep "ACTIVE" | wc -l)
	FAIL_COUT=$(grep -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "外部服务(LiveChat)-收到命令:进入会议室认证结果-失败" | wc -l)

	printf "$TIME_FORMAT_SHOW total video size: \033[32m%5s\033[0m, normal video: \033[33m%5s\033[0m, short video: \033[31m%5s\033[0m, makecall: \033[36m%5s\033[0m, man makecall: \033[34m%5s\033[0m, makecall timeout: \033[35m%5s\033[0m, makecall fail: \033[31m%5s\033[0m \n" $VIDEO_TOTAL_SIZE $VIDEO_NORMAL_COUNT $VIDEO_SHORT_COUNT $MAKECALL_COUNT $MAKECALL_MAN_COUNT $TIMEOUT_COUNT $FAIL_COUT
	
	((i++))
	((TIME=$TIME-(24*3600)))
	
done
